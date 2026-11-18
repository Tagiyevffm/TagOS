#!/usr/bin/env python3
"""Case-sensitivity and path checks for TagOS.

A Windows development host is case-insensitive: `#include "tag-system/procfs.h"`
compiles there and fails on Arch Linux. The same applies to QML imports,
`source:` file references and `install(FILES ...)` paths. This test reproduces
the Linux filesystem rules on any host so those failures surface before CI.

It also rejects files that differ only by case, which cannot be checked out on a
case-insensitive filesystem at all and would silently drop a file on Linux.

Run standalone:  python3 tests/system/test_case_and_paths.py .
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path

ERRORS: list[str] = []
SKIP_DIRS = {".git", "build", "out"}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)
QML_IMPORT_RE = re.compile(r'^\s*import\s+"([^"]+)"', re.MULTILINE)
# Only file-valued properties are references: a plain string that happens to end
# in ".qml" (e.g. `page: "AppearancePage.qml"` combined with a prefix at runtime)
# is data, not a path.
QML_FILE_RE = re.compile(
    r'(?:source|sourceComponent|image|icon|url|file)\s*:\s*"[^"$\s]*?([^"$/\s]+\.(?:qml|svg|png|ttf|otf|jpg))"'
)
CMAKE_INSTALL_RE = re.compile(r"install\(\s*(?:FILES|PROGRAMS|DIRECTORY)\s+([^)]*)\)", re.DOTALL)


def error(message: str) -> None:
    ERRORS.append(message)


def repo_root() -> Path:
    return Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]


ROOT = repo_root()


def tracked_files() -> list[Path]:
    files: list[Path] = []
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if any(part in SKIP_DIRS for part in path.relative_to(ROOT).parts):
            continue
        files.append(path)
    return files


FILES = tracked_files()
RELATIVE = {f.relative_to(ROOT).as_posix() for f in FILES}

# lowercase path -> the real paths that share it
LOWER_INDEX: dict[str, list[str]] = {}
for rel in RELATIVE:
    LOWER_INDEX.setdefault(rel.lower(), []).append(rel)


def check_case_collisions() -> None:
    for lower, paths in LOWER_INDEX.items():
        if len(paths) > 1:
            error(f"paths differ only by case (a Linux checkout keeps both, Windows drops one): {sorted(paths)}")


def exact_case(rel: str) -> str | None:
    """Returns the real path if `rel` exists with exactly this case."""
    if rel in RELATIVE:
        return rel
    return None


def report_problem(origin: str, wanted: str, hint: str = "") -> None:
    lower = wanted.lower()
    if lower in LOWER_INDEX:
        real = LOWER_INDEX[lower][0]
        error(f"{origin}: {wanted!r} does not exist with this exact case (found {real!r})")
    elif hint:
        error(f"{origin}: {wanted!r} does not exist ({hint})")


def resolve_include(origin: Path, include: str) -> None:
    # Only project-local headers matter; Qt/system headers are outside the tree.
    local_suffixes = ("tag-system/", "tag-fs/", "tag-apps/", "tag-config/",
                      "tag-control/", "tagflow/", "tag-runtime/", "tag-common/")
    is_local = include.startswith(local_suffixes) or "/" in include
    if not is_local:
        return

    origin_dir = origin.parent.relative_to(ROOT).as_posix()
    candidates = [(Path(origin_dir) / include).as_posix()]
    candidates += [rel for rel in RELATIVE if rel.endswith("/" + include)]
    if any(exact_case(c) for c in candidates):
        return
    # Name the real file when only the case differs: that is the failure mode a
    # Windows host hides.
    for rel in RELATIVE:
        if rel.lower().endswith("/" + include.lower()):
            error(f"{origin.relative_to(ROOT).as_posix()}: include {include!r} must match the file case {rel!r}")
            return
    report_problem(f"{origin.relative_to(ROOT).as_posix()}", include, "no include root provides it")


def resolve_relative(origin: Path, reference: str) -> None:
    if reference.startswith(("qrc:", "file:", "http:", "https:", "/")):
        return
    origin_dir = origin.parent.relative_to(ROOT).as_posix()
    target = os.path.join(origin_dir, reference)
    normalised = os.path.normpath(target).replace("\\", "/")
    # Path() collapses "..", so compare against the normalised form.
    if normalised in RELATIVE:
        return
    # Directories are valid QML imports too.
    if any(rel.startswith(normalised.rstrip("/") + "/") for rel in RELATIVE):
        return
    if normalised.lower() in LOWER_INDEX:
        report_problem(f"{origin.relative_to(ROOT).as_posix()}", reference)
        return
    # A relative reference may legitimately point outside the tracked set only if
    # it was produced by a generator; report it so the failure is visible.
    report_problem(f"{origin.relative_to(ROOT).as_posix()}", reference, "missing file or directory")


def check_includes_and_qml() -> None:
    for path in FILES:
        if path.suffix in {".cpp", ".h"}:
            text = path.read_text(encoding="utf-8", errors="replace")
            for include in INCLUDE_RE.findall(text):
                resolve_include(path, include)
        elif path.suffix == ".qml":
            text = path.read_text(encoding="utf-8", errors="replace")
            for module in QML_IMPORT_RE.findall(text):
                resolve_relative(path, module)
            for match in QML_FILE_RE.finditer(text):
                # Rebuild the full literal so "pages/Foo.qml" keeps its directory.
                literal = text[match.start():match.end()]
                literal = literal[literal.index('"') + 1:literal.rindex('"')]
                resolve_relative(path, literal)


def check_cmake_paths() -> None:
    for cmake in FILES:
        if cmake.name != "CMakeLists.txt":
            continue
        text = cmake.read_text(encoding="utf-8", errors="replace")
        for block in CMAKE_INSTALL_RE.findall(text):
            # Drop keywords together with their argument: COMPONENT Runtime and
            # DESTINATION <dir> are not source paths.
            block = re.sub(r"(?:COMPONENT|DESTINATION|TYPE|RENAME|PERMISSIONS)[ 	]+[^\s)]+", " ", block)
            for token in block.split():
                token = token.strip('"')
                if not token or token.startswith(("$", "DESTINATION", "COMPONENT", "TYPE",
                                                  "PERMISSIONS", "OPTIONAL", "RENAME")):
                    continue
                if token.isupper() or token.startswith("${"):
                    continue
                # Skip Qt/CMake keywords that appear inside install() argument lists.
                if token in {"DESTINATION", "FILES", "PROGRAMS", "DIRECTORY"}:
                    continue
                target = (cmake.parent.relative_to(ROOT) / token).as_posix()
                target = str(Path(target)).replace("\\", "/")
                if target in RELATIVE:
                    continue
                if any(rel.startswith(target.rstrip("/") + "/") for rel in RELATIVE):
                    continue
                if token.endswith("/"):  # directory installs may reference an existing dir
                    if any(rel.startswith(target.rstrip("/") + "/") for rel in RELATIVE):
                        continue
                report_problem(f"{cmake.relative_to(ROOT).as_posix()} install()", token, "missing path")


def check_script_line_endings() -> None:
    for path in FILES:
        if path.suffix not in {".sh", ".py", ".desktop", ".service", ".target", ".install", ".conf", ".xml", ".txt"} \
                and path.name not in {"PKGBUILD", "tag-session", "tagos-boot-marker", "autostart"}:
            continue
        data = path.read_bytes()
        if b"\r\n" in data:
            error(f"{path.relative_to(ROOT).as_posix()}: CRLF line endings break shebangs on Linux")
        if data.startswith(b"\xef\xbb\xbf"):
            error(f"{path.relative_to(ROOT).as_posix()}: UTF-8 BOM breaks shebangs and systemd units")


REQUIRED_EXECUTABLES = ["tools/build.sh", "tools/build-arch.sh", "tools/build-iso.sh",
                        "tools/run-qemu.sh", "tools/lint.sh", "tools/test-linux.sh",
                        "tools/package-arch.sh", "core/tag-session/tag-session",
                        "system/scripts/tagos-boot-marker", "tests/integration/boot-test.sh"]


def check_executable_scripts() -> None:
    """Scripts must arrive executable however the tree is delivered.

    In a git checkout the index mode (100755) is authoritative. makepkg builds
    from a `git archive` tarball that has no .git, where the tar mode is the
    truth, so there the filesystem bit is checked instead. git must never be
    consulted for a tree that is not its own repository: running `git ls-files`
    inside a tarball extracted inside another checkout lists that outer repo and
    every script looks untracked.
    """
    import os
    import subprocess

    in_git = (ROOT / ".git").exists()
    modes: dict[str, str] = {}
    if in_git:
        try:
            out = subprocess.run(["git", "-C", str(ROOT), "ls-files", "-s"],
                                 capture_output=True, text=True, check=True).stdout
        except (OSError, subprocess.CalledProcessError):
            in_git = False
        else:
            for line in out.splitlines():
                meta, _, name = line.partition("\t")
                modes[name] = meta.split()[0]

    for name in REQUIRED_EXECUTABLES:
        path = ROOT / name
        if not path.exists():
            error(f"{name}: missing")
        elif in_git:
            mode = modes.get(name)
            if mode is None:
                error(f"{name}: not tracked by git")
            elif mode != "100755":
                error(f"{name}: git mode is {mode}, expected 100755 (Linux checkout would not be executable)")
        elif not os.access(path, os.X_OK):
            error(f"{name}: not executable (a packaging tarball keeps the tar mode)")


def main() -> int:
    check_case_collisions()
    check_includes_and_qml()
    check_cmake_paths()
    check_script_line_endings()
    check_executable_scripts()

    if ERRORS:
        print(f"tagos case/path checks: {len(ERRORS)} problem(s)", file=sys.stderr)
        for message in ERRORS:
            print(f"  x {message}", file=sys.stderr)
        return 1
    print(f"tagos case/path checks: OK ({len(RELATIVE)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
