#!/usr/bin/env bash
# TagOS live/installation medium — archiso profile.
#
# This profile is intentionally thin: TagOS binaries, systemd units, labwc and
# greetd configuration are assembled into airootfs/ by tools/build-iso.sh, which
# keeps a single source of truth in system/ and the CMake install rules.

iso_name="tagos"
iso_label="TAGOS_$(date --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%Y%m)"
iso_publisher="TagOS Project <https://github.com/tagos/tagos>"
iso_application="TagOS Live / Installation Medium"
iso_version="$(date --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%Y.%m.%d)"
install_dir="arch"
buildmodes=('iso')
bootmodes=('uefi.systemd-boot')
arch="x86_64"
pacman_conf="pacman.conf"
airootfs_image_type="squashfs"
airootfs_image_tool_options=('-comp' 'xz' '-Xbcj' 'x86' '-b' '1M' '-Xdict-size' '1M')
bootstrap_tarball_compression=(zstd -c -T0 --long -19)
file_permissions=(
  ["/etc/shadow"]="0:0:400"
  ["/etc/gshadow"]="0:0:400"
  ["/etc/sudoers.d"]="0:0:750"
  ["/root"]="0:0:750"
  ["/etc/polkit-1/rules.d/49-tagos-live.rules"]="0:0:644"
)
