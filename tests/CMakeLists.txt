add_subdirectory(unit)

# ---------------------------------------------------------------------------
# Desktop smoke tests
#
# These run the real QML engine with the offscreen platform and a dry-run
# launcher: no display, no spawned applications, but every binding, import and
# component is instantiated. QML errors make the app exit non-zero.
# ---------------------------------------------------------------------------
if(TAGOS_BUILD_DESKTOP)
    add_test(NAME desktop-shell-smoke COMMAND tag-shell --smoke-test)
    set_tests_properties(desktop-shell-smoke PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;TAGOS_DRY_RUN=1"
        TIMEOUT 60
    )
endif()

# ---------------------------------------------------------------------------
# Repository / integration file checks (systemd units, archiso profile, scripts)
# ---------------------------------------------------------------------------
find_package(Python3 3.8 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME repo-system-units
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/system/test_system_files.py
                     ${PROJECT_SOURCE_DIR})
endif()
