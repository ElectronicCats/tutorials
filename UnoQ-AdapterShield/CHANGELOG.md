# Changelog

All notable changes to this project will be documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] — 2026-05-26

### Added
- One-shot on-device installer `scripts/enable-gigadisplay-shield.sh` that patches the panel module, applies a device-tree overlay, edits the boot loader entry, installs X11 rotation + calibration snippets, and reboots.
- Post-reboot verifier `scripts/validate-gigadisplay-shield.sh` with 14 checks covering board ID, kernel, panel module, DSI panel detection, `goodix_ts` binding, IRQs, and X11 input device.
- Device-tree overlay source `scripts/gigadisplay-shield.dtso`.
- Kernel patch `kernel/panel-sitronix-st7701.patch` reintroducing `arduino_giga_display_desc` for the GIGA shield panel (removed by Arduino in kernel 7.0.0).
- GitHub Actions workflow that cross-compiles the patched panel module and attaches it to the GitHub Release on every tag.
- English documentation: `README.md`, `docs/wiring.md`, `docs/how-it-works.md`, `docs/troubleshooting.md`.
- MIT license for project code and original docs; SPDX headers track GPL-2.0+ for kernel-derived files.

### Notes
- Supports Arduino UNO Q with kernel `7.0.0-g122c2c22d838`. Kernel 6.16.7 was the original development target but is not supported by this release; see `docs/how-it-works.md` "Project history" for context.
