# Kernel artifacts

This directory holds the kernel-derived inputs that produce the `panel-sitronix-st7701.ko` module shipped with each Release.

## Files

- `panel-sitronix-st7701.patch` — unified diff against `arduino/linux-qcom@qcom-v7.0.0-unoq` that reintroduces the `arduino_giga_display_desc` panel descriptor removed by Arduino in 7.0.0. Licensed `GPL-2.0+ OR BSD-3-Clause` to match the file it modifies.
- `configs/uno-q-7.0.0.config` — kernel `.config` captured from a UNO Q running `7.0.0-g122c2c22d838`. Used as the starting point for the CI cross-build.

## Building locally (maintainers only)

End users do not need to do this. The CI workflow `.github/workflows/build-panel-module.yml` produces an identical `.ko` and attaches it to every Release. Build locally only if you are iterating on the patch.

Prerequisites on the build host (Debian/Ubuntu):

```bash
sudo apt-get install -y \
  git build-essential bc cpio flex bison \
  libelf-dev libssl-dev \
  gcc-aarch64-linux-gnu ccache
```

Run the maintainer build script from the repo root:

```bash
export SSHPASS='<your-uno-q-password>'
export UNOQ_IP='<your-uno-q-ip>'
./tools/dev/build-panel-module.sh
```

The script clones the kernel, applies `panel-sitronix-st7701.patch`, configures with `configs/uno-q-7.0.0.config`, builds the full module tree (needed so `Module.symvers` is populated), and copies the resulting `panel-sitronix-st7701.ko` next to itself.

## Licensing note

The `.ko` produced from these inputs inherits GPL-2.0 from the kernel sources it was compiled against. Any redistribution must satisfy the kernel's GPL obligations — this repo satisfies them by shipping `panel-sitronix-st7701.patch` and the public CI recipe, so anyone can reproduce the binary from sources they trust.
