#!/usr/bin/env bash
# build-panel-module.sh — clones arduino/linux-qcom@qcom-v7.0.0-unoq,
# applies the patch that reintroduces arduino_giga_display_desc to the
# panel-sitronix-st7701 driver, and cross-compiles the .ko module.
#
# Output: panel-sitronix-st7701.ko (next to the script, ready for use by
# enable-gigadisplay-shield.sh).
#
# === IMPORTANT ===
# Run this script ON A DEV PC (not on the UNO Q). Requires:
#   - gcc-aarch64-linux-gnu (cross toolchain)
#   - libelf-dev, libssl-dev, flex, bison, bc, cpio
#   - The kernel .config from a UNO Q running 7.0.0 (fetched via SSH)
#   - ~5 GB of free disk space (~2 GB clone + ~3 GB build artifacts)
#
# Only needs to be re-run when Arduino updates the 7.0.0 kernel.
# The .ko shipped in the repo is built with this script.
#
# Usage:
#   export SSHPASS='<your-uno-q-password>'   # SSH password of the UNO Q
#   export UNOQ_IP='<your-uno-q-ip>'         # IP of any UNO Q running 7.0.0
#   ./tools/dev/build-panel-module.sh

set -euo pipefail

KERNEL_TARGET="7.0.0-g122c2c22d838"
KERNEL_BRANCH="qcom-v7.0.0-unoq"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-/tmp/st7701-build-$$}"
OUTPUT_KO="${SCRIPT_DIR}/panel-sitronix-st7701.ko"

UNOQ_IP="${UNOQ_IP:?set UNOQ_IP to the IP of a UNO Q running the target kernel}"
UNOQ_USER="${UNOQ_USER:-arduino}"

section() { echo; echo "==> $*"; echo "----------------------------------------------------------------"; }
info()    { echo "    $*"; }
fail()    { echo "ERROR: $*" >&2; exit 1; }

section "Prerequisite verification"
for cmd in git aarch64-linux-gnu-gcc make sshpass scp ssh python3; do
    command -v "$cmd" >/dev/null || fail "command '$cmd' not found. Install build-essential, gcc-aarch64-linux-gnu, sshpass."
done
for pkg in libelf-dev libssl-dev flex bison bc cpio; do
    dpkg -l "$pkg" 2>/dev/null | grep -q "^ii" \
      || fail "package '$pkg' missing. Install with: sudo apt-get install $pkg"
done
info "tools and deps OK"

[ -n "${SSHPASS:-}" ] || fail "export SSHPASS before running"

section "Fetching .config from UNO Q ($UNOQ_USER@$UNOQ_IP)"
mkdir -p "$BUILD_DIR"
sshpass -e scp -o StrictHostKeyChecking=accept-new \
    "${UNOQ_USER}@${UNOQ_IP}:/boot/config-${KERNEL_TARGET}" \
    "${BUILD_DIR}/config-${KERNEL_TARGET}" \
    || fail "could not fetch .config"
info "config downloaded"

section "Cloning arduino/linux-qcom branch ${KERNEL_BRANCH}"
cd "$BUILD_DIR"
[ -d linux-qcom ] || git clone --branch "$KERNEL_BRANCH" --depth 1 https://github.com/arduino/linux-qcom.git linux-qcom

section "Applying arduino,giga-display patch to the st7701 driver"
cd "$BUILD_DIR/linux-qcom"
SRC_FILE=drivers/gpu/drm/panel/panel-sitronix-st7701.c

if grep -q "arduino_giga_display_desc" "$SRC_FILE"; then
    info "patch already applied (skip)"
else
    python3 - "$SRC_FILE" <<'PYEOF'
import re, sys
f = sys.argv[1]
with open(f) as fh: src = fh.read()

GIP = r'''
/*
 * Arduino GIGA Display Shield (ASX00039) - 480x800 ST7701 panel.
 * Backported from arduino/linux-qcom branch qcom-v6.16.7-unoq (removed in
 * qcom-v7.0.0-unoq when replaced by wf40eswaa6mnn0).
 */
static void arduino_giga_display_gip_sequence(struct st7701 *st7701)
{
	ST7701_WRITE(st7701, 0xE0, 0x00, 0x00, 0x02);
	ST7701_WRITE(st7701, 0xE1, 0x08, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x09,
		0x00, 0x00, 0x33, 0x33);
	ST7701_WRITE(st7701, 0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	ST7701_WRITE(st7701, 0xE3, 0x00, 0x00, 0x33, 0x33);
	ST7701_WRITE(st7701, 0xE4, 0x44, 0x44);
	ST7701_WRITE(st7701, 0xE5, 0x0E, 0x60, 0xA0, 0xa0, 0x10, 0x60, 0xA0,
			0xA0, 0x0A, 0x60, 0xA0, 0xA0, 0x0C, 0x60, 0xA0, 0xA0);
	ST7701_WRITE(st7701, 0xE6, 0x00, 0x00, 0x33, 0x33);
	ST7701_WRITE(st7701, 0xE7, 0x44, 0x44);
	ST7701_WRITE(st7701, 0xE8, 0x0D, 0x60, 0xA0, 0xA0, 0x0F, 0x60, 0xA0,
			0xA0, 0x09, 0x60, 0xA0, 0xA0, 0x0B, 0x60, 0xA0, 0xA0);
	ST7701_WRITE(st7701, 0xEB, 0x02, 0x01, 0xE4, 0xE4, 0x44, 0x00, 0x40);
	ST7701_WRITE(st7701, 0xEC, 0x02, 0x01);
	ST7701_WRITE(st7701, 0xED, 0xAB, 0x89, 0x76, 0x54, 0x01, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0x10, 0x45, 0x67, 0x98, 0xBA);
}

'''

DESC = r'''
static const struct drm_display_mode arduino_giga_display_mode = {
	.clock		  = 27500,
	.hdisplay	  = 480,
	.hsync_start  = 400 + 100,
	.hsync_end	  = 400 + 100 + 100,
	.htotal		  = 400 + 100 + 100 + 40,
	.vdisplay	  = 800,
	.vsync_start  = 800 + 2,
	.vsync_end	  = 800 + 2 + 8,
	.vtotal		  = 800 + 2 + 8 + 17,
	.width_mm	  = 69,
	.height_mm	  = 139,
	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const struct st7701_panel_desc arduino_giga_display_desc = {
	.mode = &arduino_giga_display_mode,
	.lanes = 2,
	.format = MIPI_DSI_FMT_RGB888,
	.panel_sleep_delay = 80,
	.pv_gamma = {0x40, 0xc9, 0x91, 0x0d, 0x12, 0x07, 0x02, 0x09, 0x09,
				0x1f, 0x04, 0x50, 0x0f, 0xe4, 0x29, 0xdf},
	.nv_gamma = {0x40, 0xcb, 0xd0, 0x11, 0x92, 0x07, 0x00, 0x08, 0x07,
				0x1c, 0x06, 0x53, 0x12, 0x63, 0xeb, 0xdf},
	.nlinv = 1,
	.vop_uv = 4800000,
	.vcom_uv = 750000,
	.vgh_mv = 15000,
	.vgl_mv = -10170,
	.avdd_mv = 6600,
	.avcl_mv = -4400,
	.gamma_op_bias = OP_BIAS_MIDDLE,
	.input_op_bias = OP_BIAS_MIN,
	.output_op_bias = OP_BIAS_MIN,
	.t2d_ns = 1600,
	.t3d_ns = 10400,
	.eot_en = true,
	.gip_sequence = arduino_giga_display_gip_sequence,
};

'''

OFMATCH = '\t{ .compatible = "arduino,giga-display", .data = &arduino_giga_display_desc },\n\t{ .compatible = "sitronix,st7701", .data = &arduino_giga_display_desc },\n'

m = re.search(r'(static void wf40eswaa6mnn0_gip_sequence.*?\n\}\n)', src, re.DOTALL)
if not m: sys.exit("ERROR: no wf40 gip_sequence")
src = src[:m.end()] + GIP + src[m.end():]

m = re.search(r'(static const struct st7701_panel_desc wf40eswaa6mnn0_desc.*?\n\};\n)', src, re.DOTALL)
if not m: sys.exit("ERROR: no wf40 desc")
src = src[:m.end()] + DESC + src[m.end():]

old = '\t{ .compatible = "winstar,wf40eswaa6mnn0", .data = &wf40eswaa6mnn0_desc },\n\t{ }\n};'
new = '\t{ .compatible = "winstar,wf40eswaa6mnn0", .data = &wf40eswaa6mnn0_desc },\n' + OFMATCH + '\t{ }\n};'
if old not in src: sys.exit("ERROR: no of_match anchor")
src = src.replace(old, new)

with open(f, "w") as fh: fh.write(src)
print("patch applied OK")
PYEOF
fi

section "Build (may take 20-40 min)"
cp "${BUILD_DIR}/config-${KERNEL_TARGET}" .config
# Pin the kernel version string so the module's vermagic matches the shipped
# image EXACTLY. We force CONFIG_LOCALVERSION to the target's "-g<sha>" and turn
# CONFIG_LOCALVERSION_AUTO off, taking git fully out of scripts/setlocalversion.
# Otherwise (AUTO=y) setlocalversion appends "-dirty" — the `git apply` above
# leaves the tree modified — producing a vermagic the running kernel rejects
# with "Invalid module format". NOTE: this kernel's setlocalversion was rewritten
# and no longer reads .scmversion, so that older trick is a no-op here.
sed -i \
    -e 's/^CONFIG_LOCALVERSION=.*/CONFIG_LOCALVERSION="-g'"${KERNEL_TARGET##*-g}"'"/' \
    -e 's/^CONFIG_LOCALVERSION_AUTO=y/# CONFIG_LOCALVERSION_AUTO is not set/' \
    .config
# LOCALVERSION= (empty) stops setlocalversion appending a trailing "+" with AUTO=n.
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= olddefconfig >/dev/null
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= -j"$(nproc)" 2>&1 | tail -5

KO=drivers/gpu/drm/panel/panel-sitronix-st7701.ko
[ -f "$KO" ] || fail "build did not produce $KO"

section "Verify vermagic matches the target kernel"
VM=$(grep -a -o 'vermagic=[^[:cntrl:]]*' "$KO" | head -1)
EXPECT="vermagic=${KERNEL_TARGET} SMP preempt mod_unload aarch64"
info "built:    $VM"
info "expected: $EXPECT"
[ "$VM" = "$EXPECT" ] || fail "vermagic mismatch — this .ko would be rejected on-device with 'Invalid module format'. A '-dirty' suffix or drifting commit sha is the usual cause; confirm CONFIG_LOCALVERSION/_AUTO were forced in .config."
info "vermagic OK"

cp "$KO" "$OUTPUT_KO"
info "Output: $OUTPUT_KO"
info "  md5:  $(md5sum "$OUTPUT_KO" | cut -d' ' -f1)"
info "  size: $(stat -c%s "$OUTPUT_KO") bytes"
section "DONE"
