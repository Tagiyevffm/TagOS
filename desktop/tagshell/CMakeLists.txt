# tag-shell — the TagOS desktop shell (board, panels, dock, launcher)
#
# Phase 0 runs the whole shell as one full-screen frameless surface: wallpaper,
# top bar, side dock, bottom dock, launcher overlay and control centre. Phase 1
# splits the bars into wlr-layer-shell surfaces (see docs/roadmap.md).

qt_add_executable(tag-shell src/main.cpp)

qt_add_qml_module(tag-shell
    URI TagOS.Shell
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Wallpaper.qml
        qml/TopBar.qml
        qml/SideDock.qml
        qml/BottomDock.qml
        qml/Hero.qml
        qml/LauncherOverlay.qml
        qml/ControlCenter.qml
        qml/MonitorCard.qml
        qml/AppCatalog.qml
)

target_include_directories(tag-shell PRIVATE
    ${CMAKE_BINARY_DIR}/generated/tag-system
)
target_link_libraries(tag-shell PRIVATE
    tagos-flow
    tagos-system-qml
    tagos-design
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Svg
)

set_target_properties(tag-shell PROPERTIES
    OUTPUT_NAME "tag-shell"
    # Console subsystem on Windows dev hosts so QML warnings are visible.
    WIN32_EXECUTABLE FALSE)
tagos_set_module(tag-shell VERSION ${TAGOS_VERSION} COMPONENT desktop)
install(TARGETS tag-shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
