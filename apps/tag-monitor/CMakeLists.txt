# tag-monitor — system monitor application
#
# Phase 0 proof that a second, independent TagOS application can reuse the
# design system and the TagFlow client without touching shell code.

qt_add_executable(tag-monitor src/main.cpp)

qt_add_qml_module(tag-monitor
    URI TagOS.Monitor
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/MeterRow.qml
)

target_link_libraries(tag-monitor PRIVATE
    tagos-flow
    tagos-system-qml
    tagos-design
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Svg
)

set_target_properties(tag-monitor PROPERTIES WIN32_EXECUTABLE FALSE)
tagos_set_module(tag-monitor VERSION ${TAGOS_VERSION} COMPONENT apps)
install(TARGETS tag-monitor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
