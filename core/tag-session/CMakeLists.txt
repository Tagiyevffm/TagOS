# tag-session — session entry point
#
# The session itself is a POSIX shell script because it must not depend on Qt
# payloads being loaded before the compositor exists. It is installed by the
# top-level CMakeLists (see TAGOS_INSTALL_SYSTEM) and referenced by
# system/config/greetd/config.toml.

install(PROGRAMS tag-session DESTINATION ${CMAKE_INSTALL_BINDIR})
