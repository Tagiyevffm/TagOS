// tag-monitor — TagOS system monitor.
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTextStream>

#include "TagOSConfig.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("tag-monitor"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("TagMonitor"));
    QGuiApplication::setOrganizationName(QStringLiteral("TagOS"));
    QGuiApplication::setApplicationVersion(QStringLiteral(TAGOS_VERSION));

    QQmlApplicationEngine engine;
    const QDir appDir(QCoreApplication::applicationDirPath());
    engine.addImportPath(appDir.absolutePath());
    engine.addImportPath(QDir::cleanPath(appDir.absoluteFilePath(QStringLiteral("../lib/qml"))));
    const QString qmlPathOverride = qEnvironmentVariable("TAGOS_QML_PATH");
    if (!qmlPathOverride.isEmpty())
        engine.addImportPath(qmlPathOverride);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            QTextStream(stderr) << "tag-monitor: failed to create the window\n";
            QCoreApplication::exit(2);
        },
        Qt::QueuedConnection);

    engine.loadFromModule("TagOS.Monitor", "Main");
    if (engine.rootObjects().isEmpty())
        return 2;

    return app.exec();
}
