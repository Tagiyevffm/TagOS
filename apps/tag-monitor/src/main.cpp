// tag-monitor — TagOS system monitor entry point.
#include "tag-runtime/Runtime.h"

int main(int argc, char *argv[])
{
    TagOS::Runtime::Options options;
    options.uri = QStringLiteral("TagOS.Monitor");
    options.appName = QStringLiteral("tag-monitor");
    options.displayName = QStringLiteral("TagMonitor");
    options.version = QStringLiteral(TAGOS_VERSION);
    return TagOS::Runtime::run(argc, argv, options);
}
