# tag-system — platform metrics and system probing
#
# Pure portability layer: no UI, no D-Bus, no QML. Everything here is unit
# testable because every reader accepts an explicit path/platform shim.

qt_add_library(tagos-system STATIC
    include/tag-system/Metrics.h
    include/tag-system/ProcFs.h
    include/tag-system/CpuUsageSampler.h
    include/tag-system/OsRelease.h
    include/tag-system/Format.h
    include/tag-system/SystemProbe.h
    src/ProcFs.cpp
    src/CpuUsageSampler.cpp
    src/OsRelease.cpp
    src/Format.cpp
    src/MetricsCodec.cpp
)

target_include_directories(tagos-system
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated/tag-system>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tagos>
)

target_link_libraries(tagos-system PUBLIC Qt6::Core)

if(WIN32)
    target_sources(tagos-system PRIVATE src/SystemProbeWindows.cpp)
    target_link_libraries(tagos-system PRIVATE kernel32)
else()
    target_sources(tagos-system PRIVATE src/SystemProbeLinux.cpp)
endif()

tagos_set_module(tagos-system VERSION ${TAGOS_VERSION} COMPONENT core)

# ---------------------------------------------------------------------------
# QML surface: TagOS.System
#
# A shared library on purpose: Qt emits a QML plugin next to the module's
# qmldir, so `import TagOS.System 1.0` works from any TagOS app without
# compiling the module into each executable.
# ---------------------------------------------------------------------------
qt_add_library(tagos-system-qml SHARED)
qt_add_qml_module(tagos-system-qml
    URI TagOS.System
    VERSION 1.0
    SOURCES
        include/tag-system/PowerManager.h
        src/PowerManager.cpp
        include/tag-system/AppLauncher.h
        src/AppLauncher.cpp
)
# qmltyperegistrar emits `#include <PowerManager.h>`; make the public header
# directory resolvable by bare name for the generated registration code.
target_include_directories(tagos-system-qml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/tag-system)
target_link_libraries(tagos-system-qml PUBLIC tagos-system Qt6::Qml Qt6::Quick)

tagos_set_module(tagos-system-qml VERSION ${TAGOS_VERSION} COMPONENT core)

# Small CLI used by the boot test and by humans: `tagos-metrics --json`
qt_add_executable(tagos-metrics tools/tagos-metrics.cpp)
target_link_libraries(tagos-metrics PRIVATE tagos-system)
tagos_set_module(tagos-metrics VERSION ${TAGOS_VERSION} COMPONENT core)

install(TARGETS tagos-metrics tagos-system-qml
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
tagos_install_headers(tagos-system include/tag-system)
