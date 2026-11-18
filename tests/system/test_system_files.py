#!/usr/bin/env python3
"""Repository consistency checks for TagOS integration files.

These are the checks a Linux boot would otherwise only reveal late:

  * systemd units parse and reference binaries that this repository installs,
  * the session target actually pulls in the shell and TagFlow,
  * sandboxing is not accidentally dropped from the metrics service,
  * labwc/greetd configuration parses and points at tag-session,
  * the archiso profile is complete and references files that exist,
  * package lists are clean and duplicate-free,
  * every icon referenced from QML or from a .desktop file exists in themes/icons,
  * the wayland-sessions entry points at tag-session and is installed,
  * the PKGBUILD version tracks the CMake project version,
  * the Linux tool scripts are real, non-stub bash scripts.

Run standalone:  python3 tests/system/test_system_files.py .
Exits non-zero on the first category with problems, reporting all of them.
"""
from __future__ import annotations

import configparser
import re
import shutil
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ERRORS: list[str] = []


def error(message: str) -> None:
    ERRORS.append(message)


def repo_root() -> Path:
    if len(sys.argv) > 1:
        return Path(sys.argv[1]).resolve()
    return Path(__file__).resolve().parents[2]


ROOT = repo_root()

# Binaries this repo installs (see the install() rules in the CMakeLists files).
INSTALLED_BINARIES = {
    "tag-shell",
    "tag-flowd",
    "tag-monitor",
    "tagos-metrics",
    "tag-session",
    "tagos-boot-marker",
}

# System tools a unit may call directly.
SYSTEM_BINARIES = {
    "systemctl",
    "loginctl",
    "foot",
    "sh",
    "dbus-update-activation-environment",
    "dbus-run-session",
    "labwc",
    "tuigreet",
    "tag-session",
    "tagos-boot-marker",
}


def parse_units() -> dict[str, configparser.ConfigParser]:
    units: dict[str, configparser.ConfigParser] = {}
    for group in ("system", "user"):
        for path in sorted((ROOT / "system/systemd" / group).glob("*.*")):
            parser = configparser.ConfigParser(strict=False, interpolation=None)
            parser.optionxform = str  # systemd keys are case-sensitive
            try:
                parser.read(path, encoding="utf-8")
            except configparser.Error as exc:
                error(f"{path.relative_to(ROOT)}: cannot parse: {exc}")
                continue
            units[path.name] = parser
            if not parser.has_section("Unit"):
                error(f"{path.relative_to(ROOT)}: missing [Unit] section")
            elif not parser.get("Unit", "Description", fallback=""):
                error(f"{path.relative_to(ROOT)}: missing Description=")
    return units


def check_units(units: dict[str, configparser.ConfigParser]) -> None:
    if not units:
        error("system/systemd: no unit files found")
        return

    for name, unit in units.items():
        if not name.endswith(".service"):
            continue
        if not unit.has_section("Service"):
            error(f"{name}: missing [Service] section")
            continue
        exec_start = unit.get("Service", "ExecStart", fallback="")
        if not exec_start:
            error(f"{name}: missing ExecStart=")
            continue
        program = exec_start.split()[0]
        if not program.startswith("/"):
            error(f"{name}: ExecStart must use an absolute path ({program})")
            continue
        binary = Path(program).name
        if binary not in INSTALLED_BINARIES and binary not in SYSTEM_BINARIES:
            error(f"{name}: ExecStart references unknown binary {program}")

        service_type = unit.get("Service", "Type", fallback="simple")
        if service_type not in {"simple", "oneshot", "notify", "exec", "idle", "forking"}:
            error(f"{name}: unexpected Type={service_type}")

    # The session target is the contract between labwc autostart and the units.
    target = units.get("tag-session.target")
    if target is None:
        error("missing tag-session.target")
    else:
        wants = target.get("Unit", "Wants", fallback="")
        for dependency in ("tag-flow.service", "tag-shell.service"):
            if dependency not in wants:
                error(f"tag-session.target does not want {dependency}")
            if dependency not in units:
                error(f"tag-session.target wants {dependency}, which does not exist")

    # The metrics service is the one that reads procfs; it must stay sandboxed.
    flow = units.get("tag-flow.service")
    if flow is None:
        error("missing tag-flow.service")
    else:
        for key in ("NoNewPrivileges", "ProtectSystem", "RestrictAddressFamilies"):
            if not flow.get("Service", key, fallback=""):
                error(f"tag-flow.service: sandboxing key {key}= is missing")

    shell = units.get("tag-shell.service")
    if shell is not None:
        exec_start = shell.get("Service", "ExecStart", fallback="")
        if "--ready-file" not in exec_start:
            error("tag-shell.service must pass --ready-file so the boot test can see readiness")


def check_labwc_and_greetd() -> None:
    for name in ("rc.xml", "menu.xml"):
        path = ROOT / "system/config/labwc" / name
        if not path.exists():
            error(f"missing {path.relative_to(ROOT)}")
            continue
        try:
            ET.parse(path)
        except ET.ParseError as exc:
            error(f"{path.relative_to(ROOT)}: invalid XML: {exc}")

    autostart = ROOT / "system/config/labwc/autostart"
    if not autostart.exists():
        error("missing system/config/labwc/autostart")
    elif "tag-session.target" not in autostart.read_text(encoding="utf-8"):
        error("labwc/autostart must start tag-session.target")

    greetd = ROOT / "system/config/greetd/config.toml"
    if not greetd.exists():
        error("missing system/config/greetd/config.toml")
    else:
        text = greetd.read_text(encoding="utf-8")
        if "[default_session]" not in text:
            error("greetd config: missing [default_session]")
        if "tag-session" not in text:
            error("greetd config: default session must run tag-session")

    session = ROOT / "core/tag-session/tag-session"
    if not session.exists():
        error("missing core/tag-session/tag-session")
    else:
        text = session.read_text(encoding="utf-8")
        if "labwc" not in text:
            error("tag-session must hand over to labwc")
        if "XDG_SESSION_TYPE" not in text:
            error("tag-session must export XDG_SESSION_TYPE")


def check_archiso() -> None:
    profile = ROOT / "installer/archiso/tagos"
    profiledef = profile / "profiledef.sh"
    if not profiledef.exists():
        error("missing installer/archiso/tagos/profiledef.sh")
        return

    text = profiledef.read_text(encoding="utf-8")
    for key in ("iso_name", "iso_label", "install_dir", "bootmodes", "pacman_conf", "arch"):
        if not re.search(rf"^{key}=", text, re.MULTILINE):
            error(f"profiledef.sh: missing {key}=")

    if "uefi.systemd-boot" not in text:
        error("profiledef.sh: TagOS live media must boot with uefi.systemd-boot")

    for relative in ("pacman.conf", "packages.x86_64", "airootfs", "efiboot/loader/loader.conf"):
        if not (profile / relative).exists():
            error(f"archiso profile: missing {relative}")

    loader_conf = profile / "efiboot/loader/loader.conf"
    if loader_conf.exists():
        entries = list((profile / "efiboot/loader/entries").glob("*.conf"))
        if not entries:
            error("archiso profile: no UEFI boot entries")
        for entry in entries:
            content = entry.read_text(encoding="utf-8")
            if "archisosearchuuid=%ARCHISO_UUID%" not in content:
                error(f"{entry.name}: boot entry must search for the archiso UUID")
            if "console=ttyS0" not in content:
                error(f"{entry.name}: boot entry must keep the serial console for the boot test")

    live_greetd = profile / "airootfs/etc/greetd/config.toml"
    if live_greetd.exists():
        if "initial_session" not in live_greetd.read_text(encoding="utf-8"):
            error("live greetd config: expected [initial_session] autologin")
    else:
        error("archiso profile: missing live greetd autologin config")

    # Live media must not ship a hand-written passwd; sysusers creates the user.
    if (profile / "airootfs/etc/passwd").exists():
        error("archiso profile: use sysusers.d instead of shipping /etc/passwd")


def check_packages() -> None:
    seen: dict[str, str] = {}
    for name in ("base.txt", "desktop.txt", "dev.txt", "live.txt"):
        path = ROOT / "system/packages" / name
        if not path.exists():
            error(f"missing system/packages/{name}")
            continue
        for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if "#" in line:
                error(f"system/packages/{name}:{lineno}: inline comments are ambiguous: {line!r}")
            if not re.fullmatch(r"[a-z0-9@._+-]+", line):
                error(f"system/packages/{name}:{lineno}: invalid package name {line!r}")
            if line in seen:
                error(f"package {line} listed twice ({seen[line]} and {name})")
            seen[line] = name

    generated = ROOT / "installer/archiso/tagos/packages.x86_64"
    if generated.exists():
        lines = [l.strip() for l in generated.read_text(encoding="utf-8").splitlines() if l.strip() and not l.startswith("#")]
        if len(lines) != len(set(lines)):
            error("packages.x86_64 contains duplicates")
        missing = [p for p in ("labwc", "qt6-base", "qt6-declarative", "greetd", "pipewire", "networkmanager") if p not in lines]
        if missing:
            error(f"packages.x86_64 is missing desktop packages: {', '.join(missing)}")


def check_qml_icons() -> None:
    icons_dir = ROOT / "themes/icons"
    available = {p.stem for p in icons_dir.glob("*.svg")}
    if not available:
        error("themes/icons is empty")
        return

    # Icon names are lowercase theme files. Requiring a lowercase token also
    # keeps app *names* ("TagFiles", "Aurora") from being treated as icons, and
    # the lookbehind stops "codename:" from matching "name:".
    pattern = re.compile(r'Theme\.icon\(\s*"([a-z0-9_-]+)"\s*\)')
    icon_fields = re.compile(r'(?<![\w.-])(?:name|icon):\s*"([a-z0-9_-]+)"')
    for qml in list(ROOT.glob("desktop/**/*.qml")) + list(ROOT.glob("apps/**/*.qml")):
        text = qml.read_text(encoding="utf-8")
        referenced = set(pattern.findall(text)) | set(icon_fields.findall(text))
        for name in referenced:
            if name not in available:
                error(f"{qml.relative_to(ROOT)}: icon {name!r} not found in themes/icons")

    wallpaper = ROOT / "themes/wallpapers/aurora-dawn.svg"
    if not wallpaper.exists():
        error("missing themes/wallpapers/aurora-dawn.svg")


def desktop_entries() -> list[Path]:
    return sorted(ROOT.glob("apps/*/data/*.desktop")) + sorted(ROOT.glob("desktop/*/data/*.desktop"))


def parse_desktop_file(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("["):
            continue
        key, _, value = line.partition("=")
        values[key.strip()] = value.strip()
    return values


def check_session_entry() -> None:
    """The session entry is what a display manager lists; it must be correct."""
    entry = ROOT / "system/wayland-sessions/tagos.desktop"
    if not entry.exists():
        error("missing system/wayland-sessions/tagos.desktop")
        return

    values = parse_desktop_file(entry)
    if values.get("Type") != "Application":
        error("wayland-sessions/tagos.desktop: Type must be Application")
    if values.get("Exec") != "/usr/bin/tag-session":
        error(
            "wayland-sessions/tagos.desktop: Exec must be /usr/bin/tag-session "
            f"(got {values.get('Exec')!r})"
        )
    if values.get("TryExec") != "/usr/bin/tag-session":
        error("wayland-sessions/tagos.desktop: TryExec must point at tag-session")
    if values.get("DesktopNames") != "TagOS":
        error("wayland-sessions/tagos.desktop: DesktopNames must be TagOS")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if "wayland-sessions" not in cmake:
        error("CMakeLists.txt does not install system/wayland-sessions")
    if "icons/hicolor" not in cmake:
        error("CMakeLists.txt does not install hicolor application icons")


def check_packaging() -> None:
    pkgbuild = ROOT / "packaging/PKGBUILD"
    if not pkgbuild.exists():
        error("missing packaging/PKGBUILD")
        return

    text = pkgbuild.read_text(encoding="utf-8")
    project = re.search(r"project\(\s*TagOS\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)", (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
    pkgver = re.search(r"^pkgver=([0-9.]+)$", text, re.MULTILINE)
    if project and pkgver and project.group(1) != pkgver.group(1):
        error(f"packaging/PKGBUILD pkgver {pkgver.group(1)} != CMake project version {project.group(1)}")

    for key in ("pkgname=", "pkgrel=", "arch=", "license=", "depends=(", "makedepends=", "package()"):
        if key not in text:
            error(f"packaging/PKGBUILD: missing {key}")

    for dependency in ("labwc", "qt6-base", "qt6-declarative", "networkmanager", "pipewire"):
        if not re.search(rf"^\s*{re.escape(dependency)}\s*$", text, re.MULTILINE):
            error(f"packaging/PKGBUILD: runtime dependency {dependency} is missing")

    if "TAGOS_SYSCONF_DIR=/etc" not in text:
        error("packaging/PKGBUILD must install system configuration into /etc")

    install_script = re.search(r'^install="?\$pkgname\.install"?$', text, re.MULTILINE)
    if not install_script:
        error("packaging/PKGBUILD should reference tagos.install")
    if not (ROOT / "packaging/tagos.install").exists():
        error("missing packaging/tagos.install")


def check_tool_scripts() -> None:
    expected = {
        "build.sh",
        "build-arch.sh",
        "build-iso.sh",
        "run-qemu.sh",
        "test-linux.sh",
        "package-arch.sh",
        "lint.sh",
    }
    for name in sorted(expected):
        path = ROOT / "tools" / name
        if not path.exists():
            error(f"missing tools/{name}")
            continue
        content = path.read_text(encoding="utf-8")
        if not content.startswith("#!/usr/bin/env bash"):
            error(f"tools/{name}: expected a bash shebang")
        if "set -euo pipefail" not in content:
            error(f"tools/{name}: expected 'set -euo pipefail'")
        if len(content.splitlines()) < 10:
            error(f"tools/{name}: looks like a stub")


def check_app_icons() -> None:
    """Every .desktop Icon= must exist in themes/icons and be installed."""
    icons = {p.stem for p in (ROOT / "themes/icons").glob("*.svg")}
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    hicolor_block = cmake.split("icons/hicolor")[0].rsplit("install(FILES", 1)[-1]

    for entry in desktop_entries():
        values = parse_desktop_file(entry)
        icon = values.get("Icon", "")
        if not icon:
            error(f"{entry.relative_to(ROOT)}: missing Icon=")
            continue
        if icon not in icons:
            error(f"{entry.relative_to(ROOT)}: icon {icon!r} not found in themes/icons")
        if f"{icon}.svg" not in hicolor_block:
            error(f"{entry.relative_to(ROOT)}: icon {icon!r} is not installed into hicolor")
        # The Desktop Entry spec allows either an absolute path or a bare
        # command resolved through PATH. TagOS keeps the bare form so a dev or
        # staged build (/usr/bin or <prefix>/bin) resolves the same entry.
        command = values.get("Exec", "").split(" ")[0]
        if not command:
            error(f"{entry.relative_to(ROOT)}: missing Exec=")
        elif "/" in command and not command.startswith("/usr/bin/"):
            error(f"{entry.relative_to(ROOT)}: Exec path must stay under /usr/bin ({command!r})")


def check_docs() -> None:
    for name in ("README.md", "docs/architecture.md", "docs/phase-0.md"):
        if not (ROOT / name).exists():
            error(f"missing {name}")


def main() -> int:
    units = parse_units()
    check_units(units)
    check_labwc_and_greetd()
    check_archiso()
    check_packages()
    check_qml_icons()
    check_session_entry()
    check_packaging()
    check_tool_scripts()
    check_app_icons()
    check_docs()

    if ERRORS:
        print(f"tagos repository checks: {len(ERRORS)} problem(s)", file=sys.stderr)
        for message in ERRORS:
            print(f"  ✗ {message}", file=sys.stderr)
        return 1

    counted = {
        "units": len(units),
        "packages": sum(
            1
            for name in ("base.txt", "desktop.txt", "dev.txt", "live.txt")
            for line in (ROOT / "system/packages" / name).read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.startswith("#")
        ),
        "icons": len(list((ROOT / "themes/icons").glob("*.svg"))),
        "desktop entries": len(desktop_entries()),
    }
    print(
        "tagos repository checks: OK "
        f"({counted['units']} units, {counted['packages']} package entries, "
        f"{counted['icons']} icons, {counted['desktop entries']} desktop entries)"
    )
    return 0


if __name__ == "__main__":
    if shutil.which("git") is None:
        # Not required for the checks; mentioned only to keep the intent explicit.
        pass
    sys.exit(main())
