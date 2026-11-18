// tag-about — TagOS "About this system" entry point.
#include "tag-runtime/Runtime.h"

int main(int argc, char *argv[])
{
    TagOS::Runtime::Options options;
    options.uri = QStringLiteral("TagOS.About");
    options.appName = QStringLiteral("tag-about");
    options.displayName = QStringLiteral("TagOS Hakkında");
    options.version = QStringLiteral(TAGOS_VERSION);
    return TagOS::Runtime::run(argc, argv, options);
}
