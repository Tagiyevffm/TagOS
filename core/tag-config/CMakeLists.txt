# tag-config — persisted TagOS desktop settings (QML module TagOS.Config)
#
# One JSON document (~/.config/tagos/settings.json) is the single source of truth
# for appearance and shell behaviour. Shared library + QML module so the shell,
# TagSettings, TagFiles and tests all use the same instance-of-truth code.

qt_add_library(tagos-config SHARED)
qt_add_qml_module(tagos-config
    URI TagOS.Config
    VERSION 1.0
    SOURCES
        include/tag-config/SettingsStore.h
        src/SettingsStore.cpp
)

target_include_directories(tagos-config
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tagos>
    PRIVATE
        # qmltyperegistrar emits `#include <SettingsStore.h>`.
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tag-config
)
target_link_libraries(tagos-config PUBLIC Qt6::Core Qt6::Qml tagos-common)
target_compile_definitions(tagos-config PRIVATE TAGOS_MODULE_BUILD)

tagos_set_module(tagos-config VERSION ${TAGOS_VERSION} COMPONENT core)
install(TARGETS tagos-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
tagos_install_headers(tagos-config include/tag-config)
