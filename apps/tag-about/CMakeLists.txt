# tag-about — version, kernel, hardware and licences

qt_add_executable(tag-about src/main.cpp)

qt_add_qml_module(tag-about
    URI TagOS.About
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(tag-about PRIVATE
    tagos-runtime
    tagos-flow
    tagos-system-qml
    tagos-design
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Svg
)

set_target_properties(tag-about PROPERTIES
    OUTPUT_NAME "tag-about"
    WIN32_EXECUTABLE FALSE)
tagos_set_module(tag-about VERSION ${TAGOS_VERSION} COMPONENT apps)
install(TARGETS tag-about RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
tagos_desktop_entry(tag-about ${CMAKE_CURRENT_SOURCE_DIR}/data/tag-about.desktop)
