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

if ! command -v qmllint >/dev/null 2>&1; then
    echo "qmllint not found (install qt6-declarative / qt6-tools)" >&2
    exit 127
fi

if [[ ! -d "$QML_IMPORT_PATH" ]]; then
    echo "missing $QML_IMPORT_PATH — run tools/build.sh first" >&2
    exit 1
fi

echo "== qmllint"
while IFS= read -r -d '' file; do
    output="$(qmllint -I "$QML_IMPORT_PATH" "$file" 2>&1)" || status=1
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
        shellcheck -x "${scripts[@]}" || status=1
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
