# tag-fs — directory model, file operations, search and preview
#
# TagFiles and TagLaunch share this module. The model is stat-only (no watcher),
# every mutation returns a translated result, and nothing is deleted permanently
# without an explicit call from the UI.

qt_add_library(tagos-fs SHARED)
qt_add_qml_module(tagos-fs
    URI TagOS.Files
    VERSION 1.0
    SOURCES
        include/tag-fs/FileSystem.h
        src/FileSystem.cpp
)

target_include_directories(tagos-fs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tagos>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tag-fs
)
target_link_libraries(tagos-fs PUBLIC Qt6::Core Qt6::Gui Qt6::Qml tagos-common)
target_compile_definitions(tagos-fs PRIVATE TAGOS_MODULE_BUILD)

tagos_set_module(tagos-fs VERSION ${TAGOS_VERSION} COMPONENT core)
install(TARGETS tagos-fs
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
tagos_install_headers(tagos-fs include/tag-fs)
