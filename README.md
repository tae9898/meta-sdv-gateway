# meta-sdv-gw

Yocto custom layer for the **SDV Multi-protocol Diagnostic Gateway** on
Raspberry Pi 3: a CAN-FD / RS485 ↔ DoIP gateway with **RAUC A/B OTA**.

> Yocto Scarthgap compatible.

## What this layer provides

| Recipe | Description |
|--------|-------------|
| `sdv-gateway` (v0.1) | The gateway app (C, 4 pthreads): CAN rx, RS485 rx, DoIP rx, DoIP tx. Shared ring-buffer queue. systemd service, auto-start. Source vendored in `recipes-sdv/.../files/`. |
| `core-image-sdv-gw` | RPi3 image: systemd, openssh, can-utils, iproute2, sdv-gateway, RAUC. |
| `update-bundle` | RAUC update bundle (`.raucb`, verity, signed) for OTA. |
| `system.conf` / `boot.cmd.in` / wks / u-boot patch | RAUC A/B machinery (vendored from meta-rauc-raspberrypi). |

## Layer contents

```
meta-sdv-gw/
├── conf/layer.conf                          # LAYERDEPENDS: core meta-python rauc raspberrypi lts-u-boot-mixin
├── classes/rauc-integration.bbclass         # :rauc-integration override (active when DISTRO_FEATURES has 'rauc')
├── wic/sdimage-dual-raspberrypi.wks.in      # A/B layout: boot / rootfs_A / rootfs_B / data
├── recipes-sdv/sdv-gateway/                 # gateway app (cmake + systemd) + C source in files/
├── recipes-core/
│   ├── images/core-image-sdv-gw.bb          # custom image (WKS_FILE, rauc pkg, kernel in rootfs)
│   ├── rauc/                                # system.conf, rauc-conf/rauc bbappend, ca cert
│   ├── bundles/update-bundle.bb             # RAUC bundle recipe (verity, signed)
│   ├── base-files/                          # A/B fstab
│   └── udev/                                # don't auto-mount RAUC slots
├── recipes-bsp/
│   ├── rpi-u-boot-scr/                      # RAUC boot.cmd.in (BOOT_ORDER / BOOT_x_LEFT) + bbappend
│   └── u-boot/                              # rpi_arm64_defconfig patch (squashfs/fail-safe env)
├── recipes-kernel/linux/                    # rauc.cfg (DM_VERITY, SQUASHFS, LOOP)
└── recipes-devtools/                        # (host workarounds if needed)
```

## Dependencies (layers)

- `poky/meta`, `poky/meta-poky`
- `meta-raspberrypi`
- `meta-openembedded` (meta-oe, meta-python)
- `meta-lts-mixins` (branch `scarthgap/u-boot`) — newer U-Boot for RPi
- `meta-rauc` (branch `scarthgap`) — RAUC core + bundle.bbclass

## Usage

### `conf/bblayers.conf`
Add the dependency layers and this layer.

### `conf/local.conf` (key settings)
```bitbake
MACHINE ??= "raspberrypi3-64"
DISTRO_FEATURES:append = " systemd usrmerge rauc"
RPI_USE_U_BOOT = "1"          # RAUC needs U-Boot for A/B slot switching
ENABLE_UART = "1"             # required on RPi3 when RPI_USE_U_BOOT=1
LICENSE_FLAGS_ACCEPTED = "synaptics-killswitch"

# Cortex-A53 (ARMv8.0) cannot run PAC instructions emitted by Yocto's
# default -mbranch-protection=standard → init SIGILL. Disable it.
TUNE_CCARGS:remove = " -mbranch-protection=standard"
```

### Build
```bash
bitbake core-image-sdv-gw      # → core-image-sdv-gw-*.wic.bz2 (A/B SD image)
bitbake update-bundle          # → update-bundle-*.raucb (OTA package)
```

### Signing keys
Dev demo keys (`recipes-core/rauc/files/ca.cert.pem`, `recipes-core/bundles/files/development-1.{key,cert}.pem`)
are generated via `meta-rauc/scripts/openssl-ca.sh`. **Replace with production keys
before any real deployment** (keep the private key off the device and out of git).

## RAUC A/B OTA

- 4 partitions: `/boot` (vfat, kernel+DTB+boot.scr), `rootfs_A`, `rootfs_B`, `/data`
- U-Boot `boot.scr` reads `BOOT_ORDER` / `BOOT_x_LEFT` (written by RAUC via `fw_setenv`)
- `fw_setenv/fw_printenv` provided by `libubootenv-bin` (meta-lts-mixins u-boot does not build fw-utils)
- Auto-rollback: if a new slot fails to mark-good within N boots, bootcount falls back to the previous slot

See the top-level project README for the OTA demo and the End-to-End diagnostic test.
