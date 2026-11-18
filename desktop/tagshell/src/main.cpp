// tag-shell — TagOS desktop shell entry point.
//
// All lifecycle work (QML import paths, smoke test, screenshot, ready marker,
// logging) lives in tag-runtime so every TagOS GUI binary behaves the same.
#include <QObject>
#include <QProcessEnvironment>
#include <QVariant>

#include "tag-runtime/Runtime.h"

int main(int argc, char *argv[])
{
    TagOS::Runtime::Options options;
    options.uri = QStringLiteral("TagOS.Shell");
    options.appName = QStringLiteral("tag-shell");
    options.displayName = QStringLiteral("TagOS");
    options.version = QStringLiteral(TAGOS_VERSION);
    options.alwaysWriteReadyFile = true;
    options.afterLoad = [](QQmlApplicationEngine &, QObject *root) {
        // Dev/CI helper: open a shell surface before a screenshot is taken,
        // e.g. TAGOS_SHELL_STATE=launcher.
        const QString state = qEnvironmentVariable("TAGOS_SHELL_STATE");
        if (!state.isEmpty())
            root->setProperty("startupState", QVariant(state));
    };
    return TagOS::Runtime::run(argc, argv, options);
}
