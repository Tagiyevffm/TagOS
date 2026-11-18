# tag-runtime — shared Qt Quick application bootstrap
#
# Every TagOS GUI binary uses this instead of copy-pasting argument parsing,
# QML import paths, the smoke test and the ready-file contract.

qt_add_library(tagos-runtime STATIC
    include/tag-runtime/Runtime.h
    src/Runtime.cpp
)

target_include_directories(tagos-runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tagos>
)
target_link_libraries(tagos-runtime PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
target_compile_definitions(tagos-runtime PUBLIC TAGOS_VERSION="${TAGOS_VERSION}")

tagos_set_module(tagos-runtime VERSION ${TAGOS_VERSION} COMPONENT core)
tagos_install_headers(tagos-runtime include/tag-runtime)
