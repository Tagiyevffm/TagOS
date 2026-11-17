set(FIXTURE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)

# tagos_unit_test(<name> SOURCES ... LIBS ...)
#
# Every test is a QTest binary registered with CTest. `TAGOS_FIXTURE_DIR` points
# at plain files that mirror real /proc output, so parsers are tested without a
# live machine and without mocking.
function(tagos_unit_test name)
    cmake_parse_arguments(A "" "" "SOURCES;LIBS" ${ARGN})
    qt_add_executable(${name} ${A_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${A_LIBS})
    target_compile_definitions(${name} PRIVATE TAGOS_FIXTURE_DIR="${FIXTURE_DIR}")
    # Keep test output on the console (Qt otherwise links Windows tests as GUI apps).
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE FALSE)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 60)
endfunction()

tagos_unit_test(tst_procfs
    SOURCES tst_procfs.cpp
    LIBS tagos-system
)

tagos_unit_test(tst_cpu_sampler
    SOURCES tst_cpu_sampler.cpp
    LIBS tagos-system
)

tagos_unit_test(tst_format
    SOURCES tst_format.cpp
    LIBS tagos-system
)

tagos_unit_test(tst_osrelease
    SOURCES tst_osrelease.cpp
    LIBS tagos-system
)

tagos_unit_test(tst_metrics_codec
    SOURCES tst_metrics_codec.cpp
    LIBS tagos-system
)

tagos_unit_test(tst_flow_wire
    SOURCES tst_flow_wire.cpp
    LIBS tagos-flow
)
