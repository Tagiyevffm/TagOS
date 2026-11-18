# tag-control — real system state for TagCenter (network/audio/Bluetooth/power)
#
# Thin, honest adapters over NetworkManager, BlueZ, PipeWire, UPower,
# power-profiles-daemon, brightnessctl and rfkill. No fake data: a missing tool
# turns into `available == false` and the UI disables the tile.
# Parsers are pure and unit tested so a CLI format change fails a test instead of
# silently reporting wrong battery percentages.

qt_add_library(tagos-control SHARED)
qt_add_qml_module(tagos-control
    URI TagOS.Control
    VERSION 1.0
    SOURCES
        include/tag-control/ControlParsers.h
        include/tag-control/SystemControl.h
        src/ControlParsers.cpp
        src/SystemControl.cpp
)

target_include_directories(tagos-control
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tagos>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tag-control
)
target_link_libraries(tagos-control PUBLIC Qt6::Core Qt6::Qml tagos-common)
target_compile_definitions(tagos-control PRIVATE TAGOS_MODULE_BUILD)

tagos_set_module(tagos-control VERSION ${TAGOS_VERSION} COMPONENT core)
install(TARGETS tagos-control
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
tagos_install_headers(tagos-control include/tag-control)
