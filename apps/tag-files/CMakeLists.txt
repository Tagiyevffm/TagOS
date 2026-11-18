# tag-files — TagOS file manager
#
# Browse, search, preview and mutate the real filesystem through TagOS.Files.
# Deletion goes to the freedesktop trash; permanent deletion requires an explicit
# confirmation in the UI.

qt_add_executable(tag-files src/main.cpp)

qt_add_qml_module(tag-files
    URI TagOS.FilesApp
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(tag-files PRIVATE
    tagos-runtime
    tagos-apps
    tagos-fs
    tagos-flow
    tagos-system-qml
    tagos-design
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Svg
)

set_target_properties(tag-files PROPERTIES
    OUTPUT_NAME "tag-files"
    WIN32_EXECUTABLE FALSE)
tagos_set_module(tag-files VERSION ${TAGOS_VERSION} COMPONENT apps)
install(TARGETS tag-files RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
tagos_desktop_entry(tag-files ${CMAKE_CURRENT_SOURCE_DIR}/data/tag-files.desktop)
