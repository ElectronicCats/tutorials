# How it works

This page explains *what* the install script changes on a clean UNO Q image and *why*. Skip this if you just want a working display — go to the [README](../README.md). Read it if you plan to debug, port to a newer kernel, or audit the project before running scripts on your hardware.

## 1. The panel descriptor problem (kernel 7.0.0)

The GIGA Display Shield uses a Sitronix ST7701 panel controller driving a 480×800 DSI panel. The Linux driver `panel-sitronix-st7701.c` supports multiple variants through a *panel descriptor* table: each entry pins down the timings, GIP sequence, and connector parameters for one specific panel.

Arduino's downstream kernel for the UNO Q used to carry an `arduino_giga_display_desc` entry in that table — that's how the same driver could light up the GIGA shield. When Arduino ported their tree from `qcom-v6.16.7-unoq` to `qcom-v7.0.0-unoq`, they removed that descriptor and replaced it with a different 480×480 Winstar variant. There is no carrier overlay for the GIGA shield in 7.0.0 either, only Waveshare variants.

Result on a stock 7.0.0 install: the `panel-sitronix-st7701` module loads, but it does not know how to drive the GIGA panel. The DSI link stays uninitialised, the backlight does not even fire, and `dmesg` shows no errors because the module simply isn't matched.

**The fix** (`kernel/panel-sitronix-st7701.patch`) reintroduces the `arduino_giga_display_desc` entry and the matching `arduino_giga_display_gip_sequence` and `arduino_giga_display_mode` structs, plus two compatible-string entries (`arduino,giga-display` and the bare `sitronix,st7701` fallback) in the `of_device_id` table. The patched module is cross-compiled by CI and attached to every Release; the install script copies it on top of the stock `.ko` after backing the original up.

## 2. The device-tree overlay

The patched module exposes the GIGA panel descriptor, but the kernel still needs to be told that *this board* has *this panel* on *this DSI controller*. That's the device-tree's job.

Arduino's 7.0.0 image introduced a modular DT scheme: one base DTB (`qrb2210-arduino-imola-base.dtb`) plus carrier overlays applied at boot. The base DTB does not declare the GIGA panel, and (as noted above) there is no GIGA carrier shipped.

`scripts/gigadisplay-shield.dtso` is our overlay. It adds two things to the running tree:

- A `gt911@14` node under the `i2c@4a80000` (qup0) controller, with `interrupts-extended`, `irq-gpios`, and `reset-gpios` pointing at GPIO 98 and GPIO 18 respectively (the wiring documented in `docs/wiring.md`). The stock `goodix_ts` kernel driver matches this node and handles the touch chip.
- The panel descriptor binding: changes the DSI panel node's `compatible` to `arduino,giga-display`, which is the compatible string reintroduced by the patch in §1.

The install script compiles the `.dtso` with `dtc`, then uses `fdtoverlay` to compose it onto the base DTB, producing `/boot/efi/qrb2210-arduino-imola-gigadisplay-shield.dtb`. The composed DTB is written next to the base one — it does not overwrite it.

## 3. The boot loader entry

systemd-boot's loader entries live in `/boot/efi/loader/entries/`. The 7.0.0 entry references the base DTB by default. We do two minimal edits to the matching entry:

- Add a `devicetree /qrb2210-arduino-imola-gigadisplay-shield.dtb` line, which tells systemd-boot to load the *composed* DTB instead of the base one.
- Append `video=DSI-1:480x800@60 video=DP-1:d` to the `options` line. The first half forces the DSI connector to the panel's native 480×800@60 mode; the second half disables DP-1 so the DRM driver doesn't try to expose a phantom DisplayPort the hardware lacks.

The original entry is backed up to `.backup-pre-gigadisplay` before any edit, and `--revert` restores from it.

## 4. X11 calibration

Once `goodix_ts` is binding and `evtest` reports `ABS_MT_POSITION_X` / `Y` events, the touch chip is alive. But X11 still needs to know two things:

- **Screen rotation.** The GIGA panel is portrait-native (480×800) but we display landscape (800×480) by rotating "right". `docs/wiring.md` orientation matches that. The xorg `Monitor` section sets `Option "Rotate" "right"` on both `DSI-1` and `DP-1` because different DRM driver versions expose the connector under one name or the other.
- **Touch coordinate transform.** With the screen rotated, the touch coordinates from `goodix_ts` no longer line up with the displayed pixels. The xorg `InputClass` snippet applies a `TransformationMatrix` that rotates and translates the input space. The exact matrix `0 1.175 -0.103 -1.312 0 1.096 0 0 1` was derived empirically by capturing the four screen corners with `evtest` and solving for the affine transform that maps GT911 abs coordinates (0..479 × 0..799) to screen-space (0..799 × 0..479).

If you ever build a different panel rotation, capture four corners again and recompute the matrix; the install script writes the file, but the matrix itself is content, not infrastructure.

## 5. Project history

The current 7.0.0 approach (patched module + overlay) is the third major iteration of this project. Both earlier approaches are preserved in git history if you want to retrace the journey, but neither is supported by the public release scripts.

**Kernel 6.16.7 + DTB patch.** The first approach worked on the older 6.16.7 kernel that still carried `arduino_giga_display_desc`. We injected a `gt911@14` node into the DTB with `fdtput`. Touch initially failed — the `goodix_ts` driver could see the chip but the reset sequence was wrong. After weeks of chasing red herrings (was T8 timing missing? was `pinctrl-msm` broken?), the actual bug turned out to be inverted GPIO flags in the DTB: the patch script was emitting `GPIO_ACTIVE_LOW` for both `reset-gpios` and `irq-gpios` where upstream convention is `GPIO_ACTIVE_HIGH=0`. Fixing the flags made the stock kernel driver work on 6.16.7 without any C changes.

**Userspace daemon (Plan A).** While debugging, a Python daemon was written to drive the GT911 over `/dev/i2c-0` and inject events via `/dev/uinput`, bypassing the kernel driver entirely. It worked — but only because `libgpiod` userspace ignores DT polarity flags and operates the pins by physical levels. That masked the real bug for months. The daemon code lives in git history; it's not part of the public release because the kernel-driver path is now reliable.

**Kernel 7.0.0 + patched module + overlay (current).** When Arduino shipped the 7.0.0 image, the 6.16.7 path stopped working: Arduino had removed the panel descriptor entirely and reorganised the DT into base + carriers. The current scripts handle this by patching the module to bring the descriptor back, and using an overlay to wire the touch chip into the DT.
