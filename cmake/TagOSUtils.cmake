# Shared CMake helpers for the TagOS monorepo.
#
# Every module is expected to:
#   * own its sources, its tests/README and its version tag,
#   * expose a narrow public interface (headers under include/<module>/),
#   * never link against another module's private sources.

include_guard(GLOBAL)

# tagos_set_module(target VERSION x.y.z)
#
# Stamps a module with the monorepo version and the module's own version so the
# shell can report a per-module version table (see `tagos-metrics --modules`).
function(tagos_set_module target)
    cmake_parse_arguments(ARG "" "VERSION" "COMPONENT;DEPENDS" ${ARGN})
    if(NOT ARG_VERSION)
        set(ARG_VERSION ${TAGOS_VERSION})
    endif()
    set_target_properties(${target} PROPERTIES
        VERSION ${ARG_VERSION}
        TAGOS_MODULE_VERSION ${ARG_VERSION}
    )
    if(ARG_COMPONENT)
        set_target_properties(${target} PROPERTIES TAGOS_COMPONENT ${ARG_COMPONENT})
    endif()
endfunction()

# tagos_install_headers(target include_dir)
function(tagos_install_headers target include_dir)
    install(DIRECTORY ${include_dir}/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tagos)
endfunction()

# tagos_add_test(NAME name SOURCES ... [LIBRARIES ...] [ENVIRONMENT ...])
function(tagos_add_test)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;LIBRARIES;ENVIRONMENT" ${ARGN})
    if(NOT ARG_NAME OR NOT ARG_SOURCES)
        message(FATAL_ERROR "tagos_add_test requires NAME and SOURCES")
    endif()
    qt_add_executable(${ARG_NAME} ${ARG_SOURCES})
    target_link_libraries(${ARG_NAME} PRIVATE Qt6::Test ${ARG_LIBRARIES})
    foreach(env_pair IN LISTS ARG_ENVIRONMENT)
        set_tests_properties(${ARG_NAME} PROPERTIES ENVIRONMENT "${env_pair}")
    endforeach()
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
    return(PROPAGATE ARG_NAME)
endfunction()

# tagos_desktop_entry(target SOURCE)
#
# Installs a .desktop file for the app and copies it into the build tree so the
# development shell's app catalog (which also scans <prefix>/share/applications)
# lists the application without a full install step.
function(tagos_desktop_entry target source)
    get_filename_component(entry_name ${source} NAME)
    configure_file(${source}
                   ${CMAKE_BINARY_DIR}/share/applications/${entry_name}
                   COPYONLY)
    install(FILES ${source} DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endfunction()
