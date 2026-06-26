#!/usr/bin/env bash
# validate-gigadisplay-shield.sh — verifies that the GIGA Display
# Shield setup on the UNO Q is active and working.
#
# Run ON THE DEVICE after the reboot following
# enable-gigadisplay-shield.sh. Prints a report for each component:
# kernel, panel module, DSI panel detection, gt911 driver, IRQs, X11.
#
# Usage: ./validate-gigadisplay-shield.sh
# Read-only checks do not need sudo; dmesg/journalctl checks do.

set -u

PASS=0
FAIL=0

ok()    { echo "  ✓ $*"; PASS=$((PASS+1)); }
bad()   { echo "  ✗ $*"; FAIL=$((FAIL+1)); }
warn()  { echo "  ⚠ $*"; }
info()  { echo "    $*"; }
hdr()   { echo; echo "==> $*"; }

# ------------------------- 1. board + kernel --------------------------------
hdr "1. Board and kernel identification"

if [ -f /proc/device-tree/model ]; then
    MODEL=$(tr -d '\0' < /proc/device-tree/model)
    case "$MODEL" in
        *Arduino*) ok "Board: $MODEL" ;;
        *) bad "Board is NOT Arduino: $MODEL" ;;
    esac
else
    bad "/proc/device-tree/model does not exist"
fi

KERNEL=$(uname -r)
info "Kernel: $KERNEL"
case "$KERNEL" in
    6.16.7-g0dd6551ae96b) ok "kernel 6.16.7 recognized" ;;
    7.0.0-g122c2c22d838) ok "kernel 7.0.0 recognized" ;;
    *) warn "kernel '$KERNEL' is not a known version (did Arduino update the image?)" ;;
esac

# ------------------------- 2. kernel modules --------------------------------
hdr "2. Kernel modules"

if lsmod | grep -q "^panel_sitronix_st7701"; then
    ok "panel-sitronix-st7701 loaded"
else
    bad "panel-sitronix-st7701 NOT loaded"
    # Most common cause: the .ko vermagic does not match the running kernel
    # (e.g. a "-dirty" suffix), so it is rejected at load. Surface it directly.
    KO=/lib/modules/$(uname -r)/kernel/drivers/gpu/drm/panel/panel-sitronix-st7701.ko
    if [ -f "$KO" ]; then
        VM=$(grep -a -o 'vermagic=[^[:cntrl:]]*' "$KO" | head -1 | sed 's/^vermagic=//')
        KREL=${VM%% *}
        if [ -n "$KREL" ] && [ "$KREL" != "$(uname -r)" ]; then
            info "cause: module vermagic '$KREL' != kernel '$(uname -r)' — rejected at load (Invalid module format). Download/rebuild the matching .ko."
        fi
    fi
fi

if lsmod | grep -q "^goodix_ts"; then
    ok "goodix_ts loaded"
else
    bad "goodix_ts NOT loaded (touch driver)"
fi

# ------------------------- 3. DSI panel detection ---------------------------
hdr "3. DSI panel"

if [ -d /sys/class/drm/card0-DSI-1 ]; then
    STATUS=$(cat /sys/class/drm/card0-DSI-1/status 2>/dev/null)
    if [ "$STATUS" = "connected" ]; then
        ok "card0-DSI-1: connected"
        MODES=$(cat /sys/class/drm/card0-DSI-1/modes 2>/dev/null | head -1)
        info "Active mode: $MODES"
    else
        bad "card0-DSI-1 exists but status='$STATUS'"
    fi
elif [ -d /sys/class/drm/card0-DP-1 ]; then
    STATUS=$(cat /sys/class/drm/card0-DP-1/status 2>/dev/null)
    warn "Connector appears as DP-1 (not DSI-1) — status='$STATUS'"
    info "If the display works, update 10-monitor.conf to use DP-1"
else
    bad "No DSI-1 or DP-1 connector found in /sys/class/drm/"
    info "Connectors present: $(printf '%s ' /sys/class/drm/card* 2>/dev/null | head -c 200)"
fi

# ------------------------- 4. touch chip GT911 ------------------------------
hdr "4. Touch chip GT911"

if [ -L /sys/bus/i2c/devices/0-0014/driver ]; then
    DRV=$(basename "$(readlink /sys/bus/i2c/devices/0-0014/driver)")
    ok "Chip 0x14 bound to driver: $DRV"
else
    bad "Chip 0x14 on i2c-0 bus is NOT bound to any driver"
fi

if grep -q gt911 /proc/interrupts 2>/dev/null; then
    IRQS=$(awk '/gt911/ {sum=$2+$3+$4+$5; print sum}' /proc/interrupts)
    ok "/proc/interrupts gt911 registered, count = $IRQS"
    [ "$IRQS" -eq 0 ] 2>/dev/null && info "(0 IRQs yet — touch the screen to increment the counter)"
else
    bad "No 'gt911' entry in /proc/interrupts"
fi

if [ -e /dev/input/event2 ]; then
    NAME=$(cat /sys/class/input/event2/device/name 2>/dev/null)
    case "$NAME" in
        Goodix*) ok "Input device event2: $NAME" ;;
        *) warn "event2 exists but is not Goodix: $NAME" ;;
    esac
fi

# ------------------------- 5. installed config files ------------------------
hdr "5. Installed configuration"

[ -f /etc/X11/xorg.conf.d/10-monitor.conf ] && ok "10-monitor.conf installed" || warn "10-monitor.conf NOT installed (X11 rotation may be missing)"
[ -f /etc/X11/xorg.conf.d/20-goodix-touch.conf ] && ok "20-goodix-touch.conf installed" || warn "20-goodix-touch.conf NOT installed (X11 calibration absent)"

ENTRY=$(ls /boot/efi/loader/entries/*${KERNEL}*.conf 2>/dev/null | head -1)
if [ -n "$ENTRY" ] && grep -q "^devicetree " "$ENTRY"; then
    DTB=$(grep "^devicetree " "$ENTRY" | awk '{print $2}')
    ok "Boot entry has devicetree: $DTB"
    [ -f "/boot/efi${DTB}" ] && ok "DTB exists in /boot/efi" || bad "Referenced DTB does not exist: /boot/efi${DTB}"
else
    warn "Boot entry has no 'devicetree' line — display may not initialize"
fi

# ------------------------- 6. X11 -------------------------------------------
hdr "6. X11"

if systemctl is-active lightdm >/dev/null 2>&1; then
    ok "lightdm active"
else
    warn "lightdm NOT active (you may be in console mode)"
fi

if pgrep -x Xorg >/dev/null; then
    ok "Xorg running"
else
    warn "Xorg NOT running"
fi

# ------------------------- summary ------------------------------------------
hdr "Summary"
echo "  PASS: $PASS    FAIL: $FAIL"
echo
if [ "$FAIL" -eq 0 ]; then
    echo "  All checks passed. Touch the screen and visually confirm the cursor follows your finger."
    exit 0
else
    echo "  Errors detected. Review the output above and consult docs/troubleshooting.md."
    exit 1
fi
