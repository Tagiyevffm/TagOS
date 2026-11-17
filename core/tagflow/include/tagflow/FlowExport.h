#pragma once

#include <QtGlobal>

// TagFlow is a shared library (it also ships a QML plugin), so its C++ API is
// explicitly exported. TagOS itself runs on Linux where this is a no-op; the
// macros matter for Windows developer builds and for the QML plugin.
#if defined(TAGOS_FLOW_BUILD)
#    define TAGOS_FLOW_EXPORT Q_DECL_EXPORT
#else
#    define TAGOS_FLOW_EXPORT Q_DECL_IMPORT
#endif
