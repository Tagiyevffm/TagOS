#!/usr/bin/env bash
# Runs inside the airootfs chroot after pacstrap, i.e. when the upstream packages
# actually exist. tools/build-iso.sh cannot enable them while staging the profile:
# the overlay is assembled before pacstrap, so only TagOS's own units are present
# and `greetd.service` was silently skipped - which is why the first bootable ISO
# reached a text login instead of the desktop.
set -euo pipefail

# Session, networking and the boot marker the CI boot test greps for.
for unit in greetd.service NetworkManager.service bluetooth.service \
            systemd-timesyncd.service tagos-boot-marker.service; do
    if [[ -f "/usr/lib/systemd/system/$unit" || -f "/etc/systemd/system/$unit" ]]; then
        systemctl enable "$unit"
    else
        printf 'customize_airootfs: %s is missing from the image\n' "$unit" >&2
    fi
done

# archiso copies the profile overlay with cp -a --no-preserve=mode, so the
# executables that cmake installed into the airootfs arrive without their exec bit.
# That is why greetd's session command could never run: the file was there, the
# mode was not. Restore it here, inside the chroot, before anything uses it.
for binary in /usr/bin/tag-* /usr/lib/tagos/tagos-boot-marker; do
    [[ -f "$binary" ]] && chmod 0755 "$binary"
done

# Catch a half-populated image here instead of on a black screen.
for binary in /usr/bin/labwc /usr/bin/tag-session /usr/bin/tag-shell; do
    if [[ ! -x "$binary" ]]; then
        echo "customize_airootfs: $binary is missing or not executable" >&2
        ls -l "$binary" >&2 2>&1 || true
        exit 1
    fi
done

# Boot to the desktop session rather than a text login.
systemctl set-default graphical.target

# The live medium has no password authentication: greetd autologins the tagos
# user. Lock the root password so nothing can log in as root over the console.
passwd -l root >/dev/null 2>&1 || true

# systemd-firstboot must never prompt on the serial console; the drop-in in the
# profile handles it, this is the belt-and-braces symlink in case the drop-in is
# lost during an overlay change.
mkdir -p /etc/systemd/system
ln -sfn /dev/null /etc/systemd/system/systemd-firstboot.service
