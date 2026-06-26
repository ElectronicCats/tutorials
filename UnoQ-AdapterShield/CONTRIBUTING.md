# Contributing

Thanks for considering a contribution. This repository targets a very specific hardware setup (Arduino UNO Q + GIGA Display Shield with manual touch wiring) so most contributions will be one of: bug reports with diagnostic info, documentation fixes, support for a newer Arduino kernel version.

## Reporting an issue

Open an issue with this information so we can help quickly:

- Exact board: `cat /proc/device-tree/model`
- Kernel running: `uname -r`
- Output of `ls /boot/efi/loader/entries/`
- Output of `i2cdetect -y 0` (look for address `14`)
- Output of `lsmod | grep -E '(goodix|st7701)'`
- Output of `dmesg | grep -iE '(goodix|st7701|gt911)'` (last boot only)
- For touch problems: `timeout 5 evtest /dev/input/event* 2>&1 | head -50` while touching the screen

If you have a working setup but on different hardware, that is also useful — open an issue describing the differences.

## Submitting a pull request

1. Fork and create a feature branch off `main`.
2. Make your changes. Keep commits focused; one logical change per commit.
3. Test on real hardware — there is no substitute. Note in the PR description which board, kernel, and how you verified.
4. Run `shellcheck scripts/**/*.sh tools/**/*.sh` and fix anything it flags.
5. Do not commit binary modules (`*.ko`) — they are distributed via Releases.
6. Do not commit secrets, IPs of your dev machines, or paths under `/home/<you>/...`.
7. Open the PR against `main` and fill out the checklist in the PR template.

## Commit message style

`type(scope): short summary` in the imperative mood. Common types: `feat`, `fix`, `docs`, `chore`, `ci`. Body explains *why* the change is needed, not *what* the diff already shows.

## License

By contributing, you agree your contributions are licensed under the MIT License for original work, or GPL-2.0+ for changes to kernel-derived files (`scripts/gigadisplay-shield.dtso`, `kernel/panel-sitronix-st7701.patch`).
