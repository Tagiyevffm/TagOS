# Installer

Phase 0 ships a **bootable live medium**; the guided installation wizard is
Phase 5.

## What exists now

```
archiso/tagos/            archiso profile (UEFI systemd-boot, live autologin)
archiso/tagos/airootfs/   live-only overrides: sysusers user, greetd autologin,
                          polkit rule, volatile journal, mkinitcpio
```

`tools/build-iso.sh` assembles the rest into a staging profile:

1. builds and tests TagOS (`cmake` + `ctest`),
2. installs the binaries, QML modules, systemd units and themes with
   `DESTDIR=<stage>/airootfs cmake --install`,
3. generates `packages.x86_64` from `system/packages/{base,desktop,live}.txt`,
4. links `/etc/xdg/labwc`, `/etc/os-release` and enables the live units,
5. runs `mkarchiso` and then `tests/integration/boot-test.sh` on the result.

## Phase 5 scope

* `installer/scripts/` — partitioning (GPT + ESP + btrfs subvolumes), pacstrap of
  the TagOS package set, user/locale/timezone setup, bootloader install.
* Offline install from the live medium's package cache (the live system is
  already the target system, so `pacstrap` runs from the squashfs).
* First-boot setup: hostname, account creation, optional snapshot layout.
* The wizard is a Qt/QML application (`TagOS.Installer`) that shares
  `TagOS.Design`, and it calls a small privileged helper — never the other way
  around.
