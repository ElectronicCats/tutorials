#!/usr/bin/env bash
# enable-gigadisplay-shield.sh — enables display + touch of the Arduino GIGA
# Display Shield (ASX00039) of an Arduino UNO Q (ABX00162) on the official
# Arduino image, kernel 7.0.0-g122c2c22d838.
#
# === IMPORTANT ===
# Run this script ON THE DEVICE (not from a remote PC). It assumes you are
# logged into the UNO Q (via your own SSH, adb shell, or directly attached terminal).
#
# What it does (entirely on the current 7.0.0 kernel — no downgrade):
#   1. Installs a patched `panel-sitronix-st7701.ko` module that carries the
#      GIGA shield panel descriptor (Arduino removed it in 7.0.0 —
#      this patch reinstates it from the 6.16.7 branch).
#   2. Compiles the `gigadisplay-shield.dtso` overlay with dtc.
#   3. Applies the overlay onto the 7.0.0 base DTB with fdtoverlay,
#      producing a composed DTB that activates the DSI panel + gt911@14 node.
#   4. Edits the 7.0.0 boot loader entry to load the composed DTB and
#      configure video=DSI-1:480x800@60.
#   5. Installs xorg snippets: landscape "right" rotation + touch calibration.
#   6. Reboot.
#
# After reboot: the st7701 panel initialises with the GIGA descriptor,
# and the GT911 touch chip works with the stock `goodix_ts.ko` kernel driver.
#
# Files required next to the script (same directory):
#   - panel-sitronix-st7701.ko    (cross-compiled module from the patched kernel)
#   - gigadisplay-shield.dtso     (device-tree overlay source)
#
# Requires sudo. Asks for the password once at the start.
# Idempotent: safe to re-run; skips steps already done.
#
# Flags:
#   --no-reboot   do not reboot at the end
#   --revert      undoes EVERYTHING (restores the original module, removes the
#                 composed DTB, cleans the boot entry and xorg snippets, does NOT reboot)
#   --help        shows this help
#
# See docs/how-it-works.md for the full analysis of the underlying
# issues and why Arduino removed the panel descriptor in 7.0.0.

set -euo pipefail

# -------------------- arguments ----------------------------------------------
DO_REBOOT=1
DO_REVERT=0
for arg in "$@"; do
    case "$arg" in
        --no-reboot) DO_REBOOT=0 ;;
        --revert)    DO_REVERT=1; DO_REBOOT=0 ;;
        --help|-h)
            sed -n 's/^# \?//p' "$0" | sed -n '/^enable-gigadisplay/,/^See docs/p'
            exit 0
            ;;
        *)
            echo "ERROR: unknown argument: $arg" >&2
            echo "Use --help to see options." >&2
            exit 2
            ;;
    esac
done

# -------------------- constants ---------------------------------------------
KERNEL_TARGET="7.0.0-g122c2c22d838"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PATCHED_KO="${SCRIPT_DIR}/panel-sitronix-st7701.ko"
OVERLAY_DTSO="${SCRIPT_DIR}/gigadisplay-shield.dtso"

# Paths on the device
MODULES_DIR="/lib/modules/${KERNEL_TARGET}"
KO_TARGET="${MODULES_DIR}/kernel/drivers/gpu/drm/panel/panel-sitronix-st7701.ko"
KO_BACKUP="${KO_TARGET}.original"
BASE_DTB="/usr/lib/linux-image-${KERNEL_TARGET}/qcom/qrb2210-arduino-imola-base.dtb"
COMPOSED_DTB_NAME="qrb2210-arduino-imola-gigadisplay-shield.dtb"
COMPOSED_DTB="/boot/efi/${COMPOSED_DTB_NAME}"
ENTRIES_DIR="/boot/efi/loader/entries"
XORG_MONITOR="/etc/X11/xorg.conf.d/10-monitor.conf"
XORG_TOUCH="/etc/X11/xorg.conf.d/20-goodix-touch.conf"

# -------------------- helpers -----------------------------------------------
section() { echo; echo "==> $*"; echo "----------------------------------------------------------------"; }
info()    { echo "    $*"; }
fail()    { echo "ERROR: $*" >&2; exit 1; }

# -------------------- sanity checks -----------------------------------------
section "Initial verification"

if [ ! -f /proc/device-tree/model ] || ! grep -qi "arduino" /proc/device-tree/model 2>/dev/null; then
    fail "this script must run on an Arduino UNO Q (board not detected)."
fi
info "Board: $(tr -d '\0' < /proc/device-tree/model)"
KERNEL_RUNNING=$(uname -r)
info "Kernel running: ${KERNEL_RUNNING}"

if [ "$KERNEL_RUNNING" != "$KERNEL_TARGET" ]; then
    fail "running kernel '$KERNEL_RUNNING' ≠ target '$KERNEL_TARGET'. Boot the correct kernel and re-run."
fi

if [ "$EUID" -eq 0 ]; then
    fail "do not run as root. Run as a normal user; the script invokes sudo when needed."
fi

info "Requesting sudo privileges…"
sudo -v || fail "sudo not granted."
( while true; do sudo -nv 2>/dev/null; sleep 60; done ) &
SUDO_KEEPALIVE_PID=$!
# shellcheck disable=SC2064  # we want immediate expansion of $SUDO_KEEPALIVE_PID at trap-registration time
trap "kill $SUDO_KEEPALIVE_PID 2>/dev/null || true" EXIT

# Locate boot entry for 7.0.0
TARGET_ENTRY=$(ls "${ENTRIES_DIR}"/*${KERNEL_TARGET}*.conf 2>/dev/null | head -1) || true
[ -n "$TARGET_ENTRY" ] || fail "boot loader entry not found for $KERNEL_TARGET in $ENTRIES_DIR/"
info "Boot entry: $TARGET_ENTRY"

# -------------------- --revert: undoing everything --------------------------
if [ "$DO_REVERT" -eq 1 ]; then
    section "REVERT: undoing changes"

    # Restore the original module
    if [ -f "$KO_BACKUP" ]; then
        sudo mv "$KO_BACKUP" "$KO_TARGET"
        sudo depmod -a
        info "Original module restored: $KO_TARGET"
    fi

    # Remove devicetree + video= lines from the boot entry
    if [ -f "$TARGET_ENTRY" ]; then
        sudo sed -i "/^devicetree \/${COMPOSED_DTB_NAME}\$/d" "$TARGET_ENTRY"
        sudo sed -i "s| video=DSI-1:480x800@60 video=DP-1:d||" "$TARGET_ENTRY"
        info "Cleaned boot entry $TARGET_ENTRY"
    fi

    # Remove the composed DTB
    sudo rm -f "$COMPOSED_DTB"
    info "Removed $COMPOSED_DTB"

    # Remove xorg snippets
    sudo rm -f "$XORG_MONITOR" "$XORG_TOUCH"
    info "xorg snippets removed"

    section "REVERT complete. Reboot manually to return to the previous state."
    exit 0
fi

# -------------------- enable prerequisites ----------------------------------
[ -f "$PATCHED_KO" ] || fail "not found: $PATCHED_KO — the cross-compiled module must sit next to the script."
[ -f "$OVERLAY_DTSO" ] || fail "not found: $OVERLAY_DTSO — the overlay source must sit next to the script."
[ -f "$BASE_DTB" ] || fail "not found: $BASE_DTB — the linux-image-${KERNEL_TARGET} package may be incomplete."

for cmd in dtc fdtoverlay fdtput fdtget; do
    command -v "$cmd" >/dev/null || fail "command '$cmd' not found in PATH"
done
info "Tools OK (dtc, fdtoverlay, fdtput, fdtget)"

# Automatic backup of the boot entry
BACKUP_ENTRY="${TARGET_ENTRY}.backup-pre-gigadisplay"
if [ ! -f "$BACKUP_ENTRY" ]; then
    sudo cp "$TARGET_ENTRY" "$BACKUP_ENTRY"
    info "Boot entry backup: $BACKUP_ENTRY"
fi

# -------------------- PHASE 1: install patched module -----------------------
section "PHASE 1: install patched panel-sitronix-st7701.ko module"

# 1.0 Verify the module was built for THIS exact kernel. MODVERSIONS and
# MODULE_SIG are disabled in this kernel, so vermagic is the ONLY thing checked
# at load time — a mismatch (typically a "-dirty" suffix or a drifting commit
# sha) makes modprobe reject the module with "Invalid module format", the DSI
# panel never probes, and the screen stays black. Catch it here, not at boot.
# (modinfo is not present on the device image, so read vermagic directly.)
KO_VERMAGIC=$(grep -a -o 'vermagic=[^[:cntrl:]]*' "$PATCHED_KO" | head -1 | sed 's/^vermagic=//')
KO_KREL=${KO_VERMAGIC%% *}
if [ -z "$KO_KREL" ]; then
    fail "could not read vermagic from $PATCHED_KO — is it a valid kernel module?"
fi
if [ "$KO_KREL" != "$KERNEL_RUNNING" ]; then
    fail "the .ko was built for kernel '$KO_KREL' but this board runs '$KERNEL_RUNNING'.
       It would be rejected at load (Invalid module format) and the display would stay black.
       Download the .ko matching '$KERNEL_RUNNING' from the Release, or rebuild it
       (tools/dev/build-panel-module.sh). Full vermagic read: '$KO_VERMAGIC'."
fi
info "1.0 .ko vermagic matches running kernel: $KO_KREL"

# Back up the original .ko (idempotent)
if [ ! -f "$KO_BACKUP" ] && [ -f "$KO_TARGET" ]; then
    sudo cp "$KO_TARGET" "$KO_BACKUP"
    info "1.1 Original .ko backed up to: $KO_BACKUP"
fi

sudo cp "$PATCHED_KO" "$KO_TARGET"
sudo chown root:root "$KO_TARGET"
sudo chmod 644 "$KO_TARGET"
info "1.2 Patched .ko installed at $KO_TARGET"
info "    md5: $(md5sum "$KO_TARGET" | cut -d' ' -f1)"

sudo depmod -a
info "1.3 depmod -a OK"

# -------------------- PHASE 2: compile overlay + compose DTB ----------------
section "PHASE 2: compile overlay + compose DTB"

TMP_DTBO=$(mktemp --suffix=.dtbo)
TMP_OUT=$(mktemp --suffix=.dtb)
# shellcheck disable=SC2064  # we want immediate expansion of $TMP_DTBO, $TMP_OUT, $SUDO_KEEPALIVE_PID
trap "rm -f '$TMP_DTBO' '$TMP_OUT'; kill $SUDO_KEEPALIVE_PID 2>/dev/null || true" EXIT

# 2.1 compile dtso → dtbo
dtc -I dts -O dtb -o "$TMP_DTBO" "$OVERLAY_DTSO" 2>&1 | grep -v "^$" || true
[ -s "$TMP_DTBO" ] || fail "dtc did not produce a binary overlay"
info "2.1 Overlay compiled: $(stat -c%s "$TMP_DTBO") bytes"

# 2.2 apply overlay onto the base DTB
fdtoverlay -i "$BASE_DTB" -o "$TMP_OUT" "$TMP_DTBO" 2>&1 | grep -v "^$" || true
[ -s "$TMP_OUT" ] || fail "fdtoverlay failed to compose the DTB"
info "2.2 Composed DTB: $(stat -c%s "$TMP_OUT") bytes"

# 2.3 move composed DTB to /boot/efi/
sudo install -m 0644 "$TMP_OUT" "$COMPOSED_DTB"
info "2.3 Composed DTB installed at $COMPOSED_DTB"

# 2.4 verify the gt911 + panel nodes are present in the composed DTB
# (grep the bare quoted compatible string; the previous "compatible =" regex
# gave false negatives because dtc may render multi-string compatibles or wrap
# the property differently across versions).
COMPOSED_DTS=$(sudo dtc -I dtb -O dts "$COMPOSED_DTB" 2>/dev/null)
if printf '%s' "$COMPOSED_DTS" | grep -q '"goodix,gt911"'; then
    info "    gt911 node OK in composed DTB"
else
    info "    WARNING: gt911 not found in the composed DTB (inspect manually with dtc -I dtb -O dts $COMPOSED_DTB)"
fi
if printf '%s' "$COMPOSED_DTS" | grep -q '"arduino,giga-display"'; then
    info "    panel arduino,giga-display OK in composed DTB"
else
    info "    WARNING: panel arduino,giga-display does not appear in the DTB"
fi

# -------------------- PHASE 3: edit boot entry ------------------------------
section "PHASE 3: edit boot entry for $KERNEL_TARGET"

if grep -q "^devicetree /${COMPOSED_DTB_NAME}\$" "$TARGET_ENTRY"; then
    info "3.1 'devicetree' already present in boot entry (skip)"
else
    # First strip any previous devicetree line pointing to a different DTB
    sudo sed -i '/^devicetree /d' "$TARGET_ENTRY"
    echo "devicetree /${COMPOSED_DTB_NAME}" | sudo tee -a "$TARGET_ENTRY" >/dev/null
    info "3.1 'devicetree /${COMPOSED_DTB_NAME}' added to boot entry"
fi

if grep -q 'video=DSI-1' "$TARGET_ENTRY"; then
    info "3.2 'video=DSI-1' already in options (skip)"
else
    sudo sed -i 's|^\(options[[:space:]].*\)$|\1 video=DSI-1:480x800@60 video=DP-1:d|' "$TARGET_ENTRY"
    info "3.2 'video=DSI-1:480x800@60 video=DP-1:d' added to options"
fi

# -------------------- PHASE 4: install xorg snippets ------------------------
section "PHASE 4: xorg snippets (rotation + touch calibration)"

sudo mkdir -p /etc/X11/xorg.conf.d/

sudo tee "$XORG_MONITOR" >/dev/null <<'XORG_MONITOR_EOF'
# GIGA Display Shield rotated landscape "right".
# Use "left" for the opposite rotation.
# We define both DSI-1 and DP-1 because the kernel may expose the connector under
# either name depending on the DRM driver version.
Section "Monitor"
    Identifier "DSI-1"
    Option "Rotate" "right"
EndSection

Section "Monitor"
    Identifier "DP-1"
    Option "Rotate" "right"
EndSection

Section "Device"
    Identifier "Card0"
    Driver "modesetting"
    Option "kmsdev" "/dev/dri/card0"
EndSection

Section "Screen"
    Identifier "Screen0"
    Device "Card0"
EndSection
XORG_MONITOR_EOF
info "4.1 $XORG_MONITOR installed"

sudo tee "$XORG_TOUCH" >/dev/null <<'XORG_TOUCH_EOF'
# GT911 touch calibration for the DSI-1 display rotated "right".
# Matrix derived empirically by capturing the four corners with evtest, normalised
# to the GT911 abs_max=479/799.
Section "InputClass"
    Identifier "Goodix Touch Calibration"
    MatchProduct "Goodix Capacitive TouchScreen"
    MatchIsTouchscreen "true"
    Option "TransformationMatrix" "0 1.175 -0.103 -1.312 0 1.096 0 0 1"
EndSection
XORG_TOUCH_EOF
info "4.2 $XORG_TOUCH installed"

# -------------------- final -------------------------------------------------
section "Configuration complete"
info "Changes applied:"
info "  ✓ patched panel-sitronix-st7701.ko module at $KO_TARGET"
info "  ✓ composed DTB (base + GIGA overlay) at $COMPOSED_DTB"
info "  ✓ boot entry $TARGET_ENTRY edited (devicetree + video=DSI-1)"
info "  ✓ $XORG_MONITOR (landscape rotation)"
info "  ✓ $XORG_TOUCH (calibration)"
echo
info "Original boot entry backup: $BACKUP_ENTRY"
info "Original module backup:     $KO_BACKUP"
info "To revert EVERYTHING: $0 --revert"

if [ "$DO_REBOOT" -eq 1 ]; then
    echo
    info "Rebooting in 5 seconds for changes to take effect…"
    info "(Ctrl+C to cancel; reboot manually later with 'sudo reboot')"
    sleep 5
    sudo reboot
else
    echo
    info "--no-reboot specified. Reboot manually with 'sudo reboot' to activate the changes."
fi
