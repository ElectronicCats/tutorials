# Troubleshooting

If the install script reported success but something still doesn't work, find your symptom below.

## "Touch chip not detected by i2cdetect"

```bash
sudo i2cdetect -y 0
# No address 0x14 / 0x5d visible
```

The cause: **3.3 V power not connected to the shield.** JMEDIA pins 58 or 60 and JMISC pins 53 or 55 must reach the shield's 3V3 input. Without power, the chip never enumerates.

## "Display stays black after reboot"

The composed DTB was not picked up by the boot loader.

```bash
sudo cat /boot/efi/loader/entries/*7.0.0*.conf
```

Look for the line `devicetree /qrb2210-arduino-imola-gigadisplay-shield.dtb`. If absent, the install script's Phase 3 did not run cleanly. Re-run:

```bash
sudo ./enable-gigadisplay-shield.sh --no-reboot
sudo reboot
```

If the line is present but the screen is still black, confirm the DTB exists:

```bash
ls -la /boot/efi/qrb2210-arduino-imola-gigadisplay-shield.dtb
```

If missing, the install script's Phase 2 failed. Look for `fdtoverlay` errors in your install log.

## "evtest shows events but X11 cursor doesn't move"

The kernel sees touches, but X11 isn't routing them. Check the xorg snippet:

```bash
cat /etc/X11/xorg.conf.d/20-goodix-touch.conf
```

Should contain a `MatchProduct "Goodix Capacitive TouchScreen"` line and a `TransformationMatrix`. If missing, the install script's Phase 4 did not run.

```bash
sudo ./enable-gigadisplay-shield.sh --no-reboot
# log out of X11 and back in to reload xorg.conf.d snippets
```

## "Touch coordinates are inverted / mirrored / rotated wrong"

Open `/etc/X11/xorg.conf.d/10-monitor.conf` and try the opposite rotation:

```text
Option "Rotate" "left"
```

(Original setting is `"right"`.) Log out and back in. If neither rotation puts the cursor under your finger, the `TransformationMatrix` in `20-goodix-touch.conf` was derived for the wrong orientation — recapture the four corners with `evtest` and recompute.

## "validate-gigadisplay-shield.sh reports 1 or more FAIL"

Run it again and capture the full output. Each FAIL line names the specific check that failed; cross-reference it with the section above (touch chip, panel module, X11 device). If you can't map a FAIL to a known cause, open a issue in this repository and paste the full output.

## "Install script reports 'kernel running ≠ target'"

You are not on `7.0.0-g122c2c22d838`. This release only supports that kernel. Either:

- Boot the matching kernel from the systemd-boot menu (older entries are kept by Arduino), or
- Pin your image to the version this repo was tested against.

## "I made a mess and want to start over"

```bash
sudo ./enable-gigadisplay-shield.sh --revert
sudo reboot
```

`--revert` restores the original module from `.ko.original`, removes the composed DTB, cleans the boot entry, and removes the xorg snippets. After reboot you are back to the stock UNO Q image.
