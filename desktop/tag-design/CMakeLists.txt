# TagOS Design System (TagOS.Design)
#
# Every TagOS surface imports this module: one palette, one spacing scale, one
# motion language. Changing a token here changes the whole desktop, which is the
# point of the design system (see docs/architecture.md).

set_source_files_properties(
    qml/Theme.qml qml/Spacing.qml qml/Radius.qml qml/Typography.qml qml/Motion.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_library(tagos-design SHARED)
qt_add_qml_module(tagos-design
    URI TagOS.Design
    VERSION 1.0
    QML_FILES
        qml/Theme.qml
        qml/Spacing.qml
        qml/Radius.qml
        qml/Typography.qml
        qml/Motion.qml
        qml/TagIcon.qml
        qml/TagButton.qml
        qml/TagCard.qml
        qml/TagToggle.qml
        qml/TagRingGauge.qml
        qml/TagSparkline.qml
        qml/TagSlider.qml
        qml/TagIconButton.qml
        qml/TagChip.qml
        qml/TagListRow.qml
        qml/TagEmptyState.qml
        qml/TagMenu.qml
        qml/TagSegmented.qml
        qml/TagAppWindow.qml
)

target_link_libraries(tagos-design PRIVATE Qt6::Quick Qt6::Qml Qt6::Svg Qt6::QuickControls2)

# Theme assets are compiled into the same QML module so `qrc:/qt/qml/TagOS/Design/...`
# works identically from the build tree and from an installed image.
set(TAGOS_THEME_DIR ${PROJECT_SOURCE_DIR}/themes)
file(GLOB TAGOS_ICON_FILES CONFIGURE_DEPENDS ${TAGOS_THEME_DIR}/icons/*.svg ${TAGOS_THEME_DIR}/icons/light/*.svg)
file(GLOB TAGOS_WALLPAPER_FILES CONFIGURE_DEPENDS ${TAGOS_THEME_DIR}/wallpapers/*.svg)

qt_add_resources(tagos-design "tagos-design-assets"
    PREFIX "/qt/qml/TagOS/Design"
    BASE ${TAGOS_THEME_DIR}
    FILES ${TAGOS_ICON_FILES} ${TAGOS_WALLPAPER_FILES}
)

tagos_set_module(tagos-design VERSION ${TAGOS_VERSION} COMPONENT desktop)
install(TARGETS tagos-design
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
