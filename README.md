# Mu-Silicium for the Samsung Galaxy Tab A7 10.4 2020 (gta4l)

Fork of [Project-Silicium/Mu-Silicium](https://github.com/Project-Silicium/Mu-Silicium)
(the original README is in `README-Mu-Silicium.md`) with work on the gta4l port
(`Platforms/Samsung/gta4lPkg`, by V6lhost).

## Changes for gta4l

- **Device tree for Linux:** `DtPlatformDxe` is enabled and
  `gta4lPkg/FdtBlob/sm6115-samsung-gta4l.dtb` (from
  [limonabisi/linux-gta4l](https://github.com/limonabisi/linux-gta4l)) is
  installed as the EFI device tree, so a Linux started from UEFI gets one.
- **UEFI Shell, opened at start:** the Shell is built in and put first in the
  boot order. USB host and the USB keyboard driver were already in, so a
  keyboard on the OTG port can be used.

## Status

Not yet tested on the tablet. From the upstream status page: display,
internal storage and USB host work; power key, SD card and USB device mode do
not. Without a power key, use `reset` (reboot) or `reset -s` (power off) in the
Shell.

## Flash

`Prebuilt/gta4l/gta4l-mu-uefi.tar` → Odin, AP slot. It replaces only the boot
partition; to go back, flash your previous boot image the same way. The stock
`dtbo` must already be replaced by an empty one.

## Build

```sh
git submodule update --init --depth 1
python3 -m pip install -r pip-requirements.txt
export CLANGPDB_AARCH64_PREFIX=aarch64-linux-gnu-
python3 build_uefi.py -d gta4l        # -> Mu-gta4l.img
```

On Linux, NuGet needs `mono` for the `mu_nasm` dependency.
