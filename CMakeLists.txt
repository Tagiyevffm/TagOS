cmake_minimum_required(VERSION 3.24)

project(TagOS
    VERSION 0.1.0
    DESCRIPTION "TagOS — a modular Linux desktop distribution"
    LANGUAGES CXX
)

set(TAGOS_VERSION "${PROJECT_VERSION}")
set(TAGOS_CODENAME "Aurora")
set(TAGOS_DISPLAY_VERSION "${PROJECT_VERSION} (${TAGOS_CODENAME})")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows developer builds need every module symbol exported so tests and apps
# can link against the shared QML modules. (TagOS itself ships on Linux, where
# ELF exports everything by default.)
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# All binaries and QML modules land in <build>/bin, so `<build>/bin` is the only
# QML import path an app needs (the shell adds applicationDirPath() at startup).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# All QML modules land in <build>/bin/<Module/Uri>, next to the executables, so
# one import path (applicationDirPath) resolves every TagOS module.
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Svg Test)
# QtDBus is only available on Unix platforms; TagFlow degrades to an in-process
# backend where it is missing (Windows developer builds).
if(UNIX AND NOT APPLE)
    find_package(Qt6 6.5 QUIET COMPONENTS DBus)
endif()

if(Qt6DBus_FOUND)
    set(TAGOS_HAS_DBUS TRUE)
    set(TAGOS_FLOW_TRANSPORT "D-Bus (org.tagos.Flow1)")
else()
    set(TAGOS_HAS_DBUS FALSE)
    set(TAGOS_FLOW_TRANSPORT "local sampling (no QtDBus in this build)")
endif()

qt_standard_project_setup()

option(TAGOS_BUILD_DESKTOP "Build the TagOS desktop session (shell, apps)" ON)
option(TAGOS_BUILD_SERVICES "Build TagOS background services" ON)
option(TAGOS_BUILD_TESTS "Build TagOS tests" ON)
option(TAGOS_INSTALL_SYSTEM "Generate install rules for system integration files" ON)

include(cmake/TagOSUtils.cmake)

configure_file(
    cmake/TagOSConfig.h.in
    "${CMAKE_BINARY_DIR}/generated/tag-system/TagOSConfig.h"
    @ONLY
)

# ---------------------------------------------------------------------------
# Modules
# ---------------------------------------------------------------------------
add_subdirectory(core/tag-system)
add_subdirectory(core/tagflow)

if(TAGOS_BUILD_DESKTOP)
    add_subdirectory(desktop/tag-design)
    add_subdirectory(desktop/tagshell)
    add_subdirectory(apps/tag-monitor)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(TAGOS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Install: session, system integration, themes
# ---------------------------------------------------------------------------
if(TAGOS_INSTALL_SYSTEM)
    install(PROGRAMS core/tag-session/tag-session
            DESTINATION ${CMAKE_INSTALL_BINDIR})

    install(DIRECTORY system/systemd/user/
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user)
    install(DIRECTORY system/systemd/system/
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system)
    install(PROGRAMS system/scripts/tagos-boot-marker
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/tagos)
    install(DIRECTORY system/config/labwc/
            DESTINATION ${CMAKE_INSTALL_DATADIR}/tagos/labwc)
    install(DIRECTORY system/config/greetd/
            DESTINATION ${CMAKE_INSTALL_DATADIR}/tagos/greetd)
    install(DIRECTORY system/config/environment.d/
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/environment.d)
    install(DIRECTORY themes/
            DESTINATION ${CMAKE_INSTALL_DATADIR}/tagos/themes)
endif()

# QML modules are built into <build>/bin/TagOS/<Module> (see QT_QML_OUTPUT_DIRECTORY).
# Install that tree into <prefix>/lib/qml, which tag-shell/tag-monitor add to
# their import path relative to the executable, so any prefix works.
install(CODE "
    set(_tagos_qml_src \"${CMAKE_BINARY_DIR}/bin/TagOS\")
    if(EXISTS \"\${_tagos_qml_src}\")
        file(COPY \"\${_tagos_qml_src}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qml\")
        message(STATUS \"Installed TagOS QML modules to \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qml/TagOS\")
    endif()
")

message(STATUS "TagOS ${TAGOS_DISPLAY_VERSION} | Qt ${Qt6_VERSION} | transport: ${TAGOS_FLOW_TRANSPORT}")
