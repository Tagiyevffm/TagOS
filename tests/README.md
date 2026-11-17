# Tests

Four layers, all runnable from `ctest` except the last (which needs QEMU).

| Layer | Location | What it proves |
| --- | --- | --- |
| Unit | `tests/unit/` | parsers, sampling math, formatting and wire codecs — fast, no machine state, fixtures mirror real `/proc` output |
| Desktop | `tests/CMakeLists.txt` (`desktop-shell-smoke`) | the whole shell loads offscreen; any QML runtime error fails the run |
| Repository | `tests/system/test_system_files.py` | systemd units, labwc/greetd config, archiso profile, package lists and icon references are internally consistent |
| Integration | `tests/integration/boot-test.sh` | a real image boots under UEFI/QEMU and a TagOS session reports readiness on the serial console |

```bash
ctest --test-dir build/dev --output-on-failure          # layers 1–3
tools/lint.sh                                           # qmllint, shellcheck, repo checks
tests/integration/boot-test.sh --iso out/tagos.iso      # layer 4 (needs QEMU + OVMF)
```

## Adding a test

* Pure logic → a new `tst_*.cpp` in `tests/unit/`, registered in
  `tests/unit/CMakeLists.txt` through `tagos_unit_test()`.
* A QML component or screen → extend `desktop-shell-smoke`, or add a new
  executable with a `--smoke-test` flag that exits non-zero on QML errors.
* A system integration file → a check in `tests/system/test_system_files.py`.
* A boot path → extend `tests/integration/boot-test.sh` and the markers in
  `system/scripts/tagos-boot-marker`.
