# tag-common — headers shared by every TagOS module (no code).

add_library(tagos-common INTERFACE)
target_include_directories(tagos-common INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tagos>
)
add_library(TagOS::Common ALIAS tagos-common)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tagos)
