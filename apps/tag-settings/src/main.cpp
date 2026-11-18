// tag-settings — TagOS settings application entry point.
#include "tag-runtime/Runtime.h"

int main(int argc, char *argv[])
{
    TagOS::Runtime::Options options;
    options.uri = QStringLiteral("TagOS.Settings");
    options.appName = QStringLiteral("tag-settings");
    options.displayName = QStringLiteral("TagSettings");
    options.version = QStringLiteral(TAGOS_VERSION);
    return TagOS::Runtime::run(argc, argv, options);
}
