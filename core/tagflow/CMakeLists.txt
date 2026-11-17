# tag-flow — the TagOS IPC layer for system metrics
#
# Two pieces:
#   * libtagos-flow  : FlowClient (QML type TagOS.Flow.FlowClient) + transports
#   * tag-flowd      : session service implementing org.tagos.Flow1 (Linux)
#
# Clients never touch procfs: they bind to FlowClient, which talks D-Bus when
# tag-flowd is alive and samples locally otherwise.

qt_add_library(tagos-flow SHARED)
qt_add_qml_module(tagos-flow
    URI TagOS.Flow
    VERSION 1.0
    SOURCES
        include/tagflow/FlowExport.h
        include/tagflow/TagFlowTypes.h
        include/tagflow/Backend.h
        include/tagflow/FlowClient.h
        src/TagFlowTypes.cpp
        src/Backends.cpp
        src/FlowClient.cpp
    RESOURCES
        dbus/org.tagos.Flow1.xml
)
target_include_directories(tagos-flow PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
# Generated QML registration code includes the headers by bare name.
target_include_directories(tagos-flow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/tagflow)
target_link_libraries(tagos-flow PUBLIC tagos-system Qt6::Qml Qt6::Quick)
target_compile_definitions(tagos-flow PRIVATE TAGOS_FLOW_BUILD)

if(TAGOS_HAS_DBUS)
    target_sources(tagos-flow PRIVATE src/DbusBackend.cpp)
    target_link_libraries(tagos-flow PRIVATE Qt6::DBus)
    target_compile_definitions(tagos-flow PRIVATE TAGOS_HAS_DBUS=1)
endif()

# Windows developer hosts: export the C++ API so tests can link against the
# shared QML module (ELF exports everything by default on TagOS itself).
if(WIN32)
    set_target_properties(tagos-flow PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

tagos_set_module(tagos-flow VERSION ${TAGOS_VERSION} COMPONENT core)

if(TAGOS_BUILD_SERVICES AND TAGOS_HAS_DBUS)
    qt_add_executable(tag-flowd
        daemon/main.cpp
        daemon/FlowService.h
        daemon/FlowService.cpp
    )
    target_link_libraries(tag-flowd PRIVATE tagos-flow tagos-system Qt6::DBus)
    tagos_set_module(tag-flowd VERSION ${TAGOS_VERSION} COMPONENT core)
    install(TARGETS tag-flowd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS tagos-flow
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
tagos_install_headers(tagos-flow include/tagflow)
