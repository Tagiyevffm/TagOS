// tag-files — TagOS file manager entry point.
#include "tag-runtime/Runtime.h"

int main(int argc, char *argv[])
{
    TagOS::Runtime::Options options;
    options.uri = QStringLiteral("TagOS.FilesApp");
    options.appName = QStringLiteral("tag-files");
    options.displayName = QStringLiteral("TagFiles");
    options.version = QStringLiteral(TAGOS_VERSION);
    return TagOS::Runtime::run(argc, argv, options);
}
