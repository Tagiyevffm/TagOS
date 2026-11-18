#!/usr/bin/env bash
# Lint every QML file with qmllint, every script with shellcheck and the
# repository integration files with tests/system/test_system_files.py.
#
# qmllint resolves TagOS modules through the build tree, so run `tools/build.sh`
# (or at least the cmake configure step) first.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TAGOS_BUILD_DIR:-$ROOT/build/dev}"
QML_IMPORT_PATH="$BUILD_DIR/bin"
status=0

# qmllint (Qt 6.x) does not introspect the properties of QML-declared singleton
# types, so every `Theme.accent` / `Spacing.lg` read is reported as
# "missing-property" together with a bogus "not declared as singleton" note.
# The singletons themselves are covered by the desktop smoke test, which fails
# on real QML runtime errors (TypeError, unassignable bindings). Filter exactly
# those two classes so every other warning stays visible and useful.
filter_known_qmllint_noise() {
    awk '
        /^Warning:/ && (/missing-property/ || /not declared as singleton/) { skip = 1; next }
        /^Warning:|^Info:|^Error:/ { skip = 0 }
        skip && (/^[[:space:]]/ || /^\^/) { next }
        { if (!skip) print }
    '
}

# Arch installs qmllint as /usr/lib/qt6/bin/qmllint: the Qt tools live in a
# libexec-style directory that is not on PATH, while /usr/bin only carries the
# versioned wrappers (qml6, qmlscene6, ...). Ask qtpaths first, then fall back to
# the known layouts, so no distro prefix is hard-coded.
QMLLINT="${TAGOS_QMLLINT:-}"
if [[ -z "$QMLLINT" ]]; then
    if command -v qmllint >/dev/null 2>&1; then
        QMLLINT="$(command -v qmllint)"
    else
        candidates=()
        if command -v qtpaths6 >/dev/null 2>&1; then
            for key in QT_HOST_LIBEXECS QT_INSTALL_BINS QT_HOST_BINS; do
                dir="$(qtpaths6 --query "$key" 2>/dev/null)" || continue
                [[ -n "$dir" ]] && candidates+=("$dir/qmllint" "$dir/bin/qmllint")
            done
        fi
        candidates+=(/usr/lib/qt6/bin/qmllint /usr/lib/qt6/qmllint /usr/lib64/qt6/bin/qmllint)
        for candidate in "${candidates[@]}"; do
            if [[ -x "$candidate" ]]; then
                QMLLINT="$candidate"
                break
            fi
        done
        # Last resort: a shallow search under the Qt library directory.
        if [[ -z "$QMLLINT" ]]; then
            QMLLINT="$(find /usr/lib/qt6 /usr/lib64/qt6 -maxdepth 3 -type f -name qmllint -print -quit 2>/dev/null)"
        fi
    fi
fi
if [[ -z "$QMLLINT" ]]; then
    echo "qmllint not found (install qt6-declarative / qt6-tools, or set TAGOS_QMLLINT)" >&2
    exit 127
fi
echo "   qmllint: $QMLLINT"

if [[ ! -d "$QML_IMPORT_PATH" ]]; then
    echo "missing $QML_IMPORT_PATH — run tools/build.sh first" >&2
    exit 1
fi

echo "== qmllint"
while IFS= read -r -d '' file; do
    output="$("$QMLLINT" -I "$QML_IMPORT_PATH" "$file" 2>&1)" || status=1
    filtered="$(printf '%s\n' "$output" | filter_known_qmllint_noise)"
    if [[ -n "${filtered//[[:space:]]/}" ]]; then
        printf '%s\n' "$filtered"
    fi
done < <(find "$ROOT/desktop" "$ROOT/apps" -name '*.qml' -print0)

echo "== shellcheck"
if command -v shellcheck >/dev/null 2>&1; then
    scripts=()
    while IFS= read -r -d '' script; do
        scripts+=("$script")
    done < <(find "$ROOT/tools" "$ROOT/tests" "$ROOT/system/scripts" "$ROOT/core/tag-session" "$ROOT/installer" \
                  -type f \( -name '*.sh' -o -name 'tag-session' -o -name 'tagos-boot-marker' \) -print0)
    if [[ ${#scripts[@]} -gt 0 ]]; then
        # --severity=warning keeps real defects fatal while ignoring info-level
        # noise (unfollowable system imports, ls-vs-find preferences).
        shellcheck -x --severity=warning "${scripts[@]}" || status=1
    fi
else
    echo "shellcheck not installed, skipping"
fi

echo "== repository checks"
if command -v python3 >/dev/null 2>&1; then
    python3 "$ROOT/tests/system/test_system_files.py" "$ROOT" || status=1
elif command -v python >/dev/null 2>&1; then
    python "$ROOT/tests/system/test_system_files.py" "$ROOT" || status=1
else
    echo "python not found, skipping repository checks" >&2
    status=1
fi

if [[ $status -eq 0 ]]; then
    echo "== lint OK"
fi
exit "$status"
