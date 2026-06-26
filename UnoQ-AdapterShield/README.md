# Arduino UNO Q + GIGA Display Shield — Full Linux Support

End-to-end support for the **Arduino GIGA Display Shield** (ASX00039) — 480×800 DSI panel and GT911 capacitive touch — running on the **Arduino UNO Q** (ABX00162, Qualcomm QRB2210, Debian-based Arduino image, kernel 7.0.0).

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Kernel: 7.0.0](https://img.shields.io/badge/kernel-7.0.0--g122c2c22d838-informational)](kernel/README.md)
[![CI](https://github.com/sabas1080/i2c_arduinoQ/actions/workflows/build-panel-module.yml/badge.svg)](https://github.com/sabas1080/i2c_arduinoQ/actions/workflows/build-panel-module.yml)

## What this does

One on-device script:

- Installs a **patched `panel-sitronix-st7701.ko`** that brings back the GIGA panel descriptor Arduino removed in kernel 7.0.0.
- Applies a **device-tree overlay** that adds the GT911 touch node and binds the patched panel.
- Edits the **systemd-boot entry** to load the composed DTB and pin the DSI mode to 480×800@60.
- Installs **xorg snippets** for landscape rotation and touch calibration.

Run, reboot, the screen lights up, the touch works.

## Status

| Feature | Kernel 7.0.0-g122c2c22d838 |
|---|---|
| DSI panel (480×800) | ✓ patched panel-sitronix-st7701 module |
| GT911 touch (I²C 0x14) | ✓ stock `goodix_ts` driver via overlay |
| X11 cursor (landscape, calibrated) | ✓ |

> Kernel 6.16.7 was the original development target and is no longer supported by this release. The 6.16.7 path remains in git history; see [`docs/how-it-works.md` § Project history](docs/how-it-works.md#5-project-history).

## Quick start

You will need:
- An Arduino UNO Q running the official Arduino image with kernel `7.0.0-g122c2c22d838` (default on current images).
- An Adapter Shield for Arduino UNO Q + Arduino GIGA Display Shield. See [`HW`](HW).
- An Arduino GIGA Display Shield.

Steps:

1. **Assemble the boards together.** Verify with `i2cdetect -y 0` — address `0x14` should appear.
2. **Download the latest release bundle** from [Releases](https://github.com/ElectronicCats/tutorials/releases/latest):
   - `enable-gigadisplay-shield.sh`
   - `validate-gigadisplay-shield.sh`
   - `gigadisplay-shield.dtso`
   - `panel-sitronix-st7701-7.0.0-g122c2c22d838.ko`
   - `SHA256SUMS`
3. **Copy to the UNO Q** by your preferred method (scp / adb push / USB stick), keeping all four files in the same directory.
4. **Verify the .ko checksum:**
   ```bash
   sha256sum -c SHA256SUMS
   ```
5. **Rename the .ko** so the install script finds it:
   ```bash
   mv panel-sitronix-st7701-*.ko panel-sitronix-st7701.ko
   ```
6. **Run the installer** (asks for sudo once):
   ```bash
   chmod +x enable-gigadisplay-shield.sh validate-gigadisplay-shield.sh
   ./enable-gigadisplay-shield.sh
   ```
   It reboots when done.
7. **After reboot, verify:**
   ```bash
   ./validate-gigadisplay-shield.sh
   ```
8. Expect `14/14 PASS`. Anything less, see [`docs/troubleshooting.md`](docs/troubleshooting.md).

## How it works

Three pieces stitched together:

1. The `panel-sitronix-st7701` driver gets a patch that reintroduces the GIGA panel descriptor Arduino dropped in 7.0.0.
2. A device-tree overlay adds the GT911 touch node and points the DSI panel at the new descriptor.
3. The boot loader entry loads the composed DTB and pins the DSI mode.

Full explanation with rationale: [`docs/how-it-works.md`](docs/how-it-works.md).

## Reverting

```bash
./enable-gigadisplay-shield.sh --revert
sudo reboot
```

Restores the original panel module, removes the composed DTB, cleans the boot entry, removes the xorg snippets.

## Troubleshooting

See [`docs/troubleshooting.md`](docs/troubleshooting.md) — indexed by symptom.

## Licensing

- **Project license: MIT** for original code and documentation (`LICENSE`).
- **`scripts/gigadisplay-shield.dtso`** is licensed `GPL-2.0+ OR BSD-3-Clause` per the kernel DTS convention (SPDX header at the top of the file).
- **`kernel/panel-sitronix-st7701.patch`** is licensed `GPL-2.0+ OR BSD-3-Clause` because it derives from the upstream `panel-sitronix-st7701.c` driver.
- **The `.ko` binary** distributed via GitHub Releases is GPL-2.0 (compiled from the GPL kernel sources). Redistribution must satisfy the kernel's GPL obligations — this repo satisfies them by shipping the patch and the public CI recipe.

If you reuse pieces of this repo, MIT covers the scripts and docs cleanly; the kernel-derived files carry GPL obligations independent of the MIT umbrella.

## Credits

- **Electronic Cats** — host community, hardware support.
- **Arduino** — official UNO Q image and kernel source ([arduino/linux-qcom](https://github.com/arduino/linux-qcom)).
- **Goodix** — GT911 datasheet.
- **Linux drm-misc** — upstream `panel-sitronix-st7701` driver.

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md).
