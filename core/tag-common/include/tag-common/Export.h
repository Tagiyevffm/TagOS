#pragma once

// TagOS symbol visibility macro.
//
// TagOS ships on Linux, where the dynamic linker exports everything by default.
// Windows developer builds need explicit exports, so every TagOS shared library
// compiles with TAGOS_MODULE_BUILD defined and annotates the classes/functions
// that other modules and the tests link against.
#if defined(_WIN32) || defined(__CYGWIN__)
#    if defined(TAGOS_MODULE_BUILD)
#        define TAGOS_EXPORT __declspec(dllexport)
#    else
#        define TAGOS_EXPORT __declspec(dllimport)
#    endif
#else
#    define TAGOS_EXPORT __attribute__((visibility("default")))
#endif
