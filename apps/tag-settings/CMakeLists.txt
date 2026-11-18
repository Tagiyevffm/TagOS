# tag-settings — TagOS system settings
#
# Six pages, all backed by real state: SettingsStore (appearance), SystemControl
# (network/audio/power/BT), SessionInfo (user/system) and PowerManager (logout,
# reboot, poweroff through logind).

qt_add_executable(tag-settings src/main.cpp)

qt_add_qml_module(tag-settings
    URI TagOS.Settings
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/SettingRow.qml
        qml/pages/AppearancePage.qml
        qml/pages/NetworkPage.qml
        qml/pages/SoundPage.qml
        qml/pages/PowerPage.qml
        qml/pages/UserPage.qml
        qml/pages/SystemPage.qml
)

target_link_libraries(tag-settings PRIVATE
    tagos-runtime
    tagos-config
    tagos-control
    tagos-flow
    tagos-system-qml
    tagos-design
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Svg
)

set_target_properties(tag-settings PROPERTIES
    OUTPUT_NAME "tag-settings"
    WIN32_EXECUTABLE FALSE)
tagos_set_module(tag-settings VERSION ${TAGOS_VERSION} COMPONENT apps)
install(TARGETS tag-settings RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
tagos_desktop_entry(tag-settings ${CMAKE_CURRENT_SOURCE_DIR}/data/tag-settings.desktop)
