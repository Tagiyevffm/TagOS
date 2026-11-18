# tag-apps — desktop entry parsing and the TagOS application catalog
#
# TagLaunch, the dock and TagSettings all read applications from here. Parsing is
# pure and fixture-tested; the catalog merges XDG directories by id.

qt_add_library(tagos-apps SHARED)
qt_add_qml_module(tagos-apps
    URI TagOS.Apps
    VERSION 1.0
    SOURCES
        include/tag-apps/DesktopEntry.h
        include/tag-apps/AppCatalog.h
        src/DesktopEntry.cpp
        src/AppCatalog.cpp
)

target_include_directories(tagos-apps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tagos>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tag-apps
)
target_link_libraries(tagos-apps PUBLIC Qt6::Core Qt6::Qml tagos-common tagos-system tagos-system-qml)
target_compile_definitions(tagos-apps PRIVATE TAGOS_MODULE_BUILD)

tagos_set_module(tagos-apps VERSION ${TAGOS_VERSION} COMPONENT core)
install(TARGETS tagos-apps
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
tagos_install_headers(tagos-apps include/tag-apps)
