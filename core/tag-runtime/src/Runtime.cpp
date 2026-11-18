#include "tag-runtime/Runtime.h"

#include <cstdio>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>

namespace TagOS::Runtime {
namespace {

QString g_logFile;
int g_qmlErrors = 0;

/// QML runtime failures (undefined tokens, failed bindings) only produce log
/// warnings, so "did the file load?" would pass on a broken UI. Count them.
bool isQmlRuntimeError(const QString &message)
{
    static const QLatin1StringView needles[] = {
        QLatin1StringView("TypeError"),
        QLatin1StringView("is not defined"),
        QLatin1StringView("is not a function"),
        QLatin1StringView("Cannot assign"),
        QLatin1StringView("Unable to assign"),
        QLatin1StringView("Non-existent attached object"),
        QLatin1StringView("Cannot read property"),
        QLatin1StringView("is not a type"),
        QLatin1StringView("Cannot assign to non-existent property"),
        QLatin1StringView("Unable to assign"),
    };
    for (const auto &needle : needles) {
        if (message.contains(needle))
            return true;
    }
    return false;
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (isQmlRuntimeError(message))
        ++g_qmlErrors;

    const QByteArray line = qFormatLogMessage(type, context, message).toUtf8() + '\n';
    std::fwrite(line.constData(), 1, static_cast<size_t>(line.size()), stderr);
    std::fflush(stderr);
    if (!g_logFile.isEmpty()) {
        QFile file(g_logFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            file.write(line);
    }
}

QString resolveReadyFile(const QString &requested)
{
    if (!requested.isEmpty())
        return requested;
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString base = runtime.isEmpty() ? QDir::tempPath() : runtime;
    return base + QStringLiteral("/tagos/") + QStringLiteral("ready");
}

bool writeReadyFile(const QString &path, const Options &options)
{
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath()))
        return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    const QJsonObject payload{
        {QStringLiteral("component"), options.appName},
        {QStringLiteral("version"), options.version},
        {QStringLiteral("host"), QSysInfo::machineHostName()},
        {QStringLiteral("platform"), QGuiApplication::platformName()},
        {QStringLiteral("pid"), static_cast<double>(QCoreApplication::applicationPid())},
    };
    file.write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    return true;
}

void addImportPaths(QQmlApplicationEngine &engine)
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    engine.addImportPath(appDir.absolutePath());
    engine.addImportPath(QDir::cleanPath(appDir.absoluteFilePath(QStringLiteral("../lib/qml"))));
    const QString override = qEnvironmentVariable("TAGOS_QML_PATH");
    if (!override.isEmpty())
        engine.addImportPath(override);
}

} // namespace

int run(int argc, char *argv[], const Options &options)
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("TagOS"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("tagos.org"));
    QGuiApplication::setApplicationName(options.appName);
    QGuiApplication::setApplicationDisplayName(options.displayName);
    QGuiApplication::setApplicationVersion(options.version);
    QGuiApplication::setDesktopFileName(options.appName);

    QCommandLineParser parser;
    parser.setApplicationDescription(options.displayName + QStringLiteral(" — TagOS"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption readyOption(QStringLiteral("ready-file"),
                                   QStringLiteral("Write a session-ready marker to <path>."),
                                   QStringLiteral("path"));
    QCommandLineOption smokeOption(QStringLiteral("smoke-test"),
                                   QStringLiteral("Load the UI, then exit 0 (used by ctest/CI)."));
    QCommandLineOption logOption(QStringLiteral("log-file"),
                                 QStringLiteral("Append Qt diagnostics to <path>."),
                                 QStringLiteral("path"));
    QCommandLineOption screenshotOption(QStringLiteral("screenshot"),
                                        QStringLiteral("Render one frame to <path> and exit."),
                                        QStringLiteral("path"));
    parser.addOption(readyOption);
    parser.addOption(smokeOption);
    parser.addOption(logOption);
    parser.addOption(screenshotOption);
    parser.process(app);

    g_logFile = parser.isSet(logOption)
        ? parser.value(logOption)
        : QProcessEnvironment::systemEnvironment().value(QStringLiteral("TAGOS_LOG_FILE"));
    qInstallMessageHandler(messageHandler);

    QQmlApplicationEngine engine;
    addImportPaths(engine);

    bool loadFailed = false;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [&loadFailed]() {
            loadFailed = true;
            QTextStream(stderr) << "tagos: failed to create the application window\n";
            QCoreApplication::exit(2);
        },
        Qt::QueuedConnection);

    engine.loadFromModule(options.uri, QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty() || loadFailed)
        return 2;

    QObject *root = engine.rootObjects().constFirst();
    if (options.afterLoad)
        options.afterLoad(engine, root);

    if (parser.isSet(readyOption) || options.alwaysWriteReadyFile) {
        const QString readyFile = resolveReadyFile(parser.value(readyOption));
        if (!writeReadyFile(readyFile, options))
            QTextStream(stderr) << options.appName << ": could not write ready file " << readyFile << '\n';
    }

    if (parser.isSet(screenshotOption)) {
        const QString target = parser.value(screenshotOption);
        QTimer::singleShot(1400, &app, [&engine, target, &app]() {
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
            if (!window) {
                QTextStream(stderr) << "tagos: no window to render\n";
                QCoreApplication::exit(4);
                return;
            }
            const QImage frame = window->grabWindow();
            if (frame.isNull() || !frame.save(target)) {
                QTextStream(stderr) << "tagos: could not save screenshot to " << target << '\n';
                QCoreApplication::exit(5);
                return;
            }
            QTextStream(stdout) << "TAGOS_SCREENSHOT " << target << ' ' << frame.width() << 'x'
                                << frame.height() << '\n';
            app.quit();
        });
    }

    if (parser.isSet(smokeOption)) {
        QTimer::singleShot(900, &app, [&app]() {
            if (g_qmlErrors > 0) {
                QTextStream(stdout) << "TAGOS_SMOKE_FAILED qml-errors=" << g_qmlErrors << '\n';
                app.exit(3);
                return;
            }
            QTextStream(stdout) << "TAGOS_SMOKE_OK " << QCoreApplication::applicationName() << ' '
                                << QCoreApplication::applicationVersion() << '\n';
            app.quit();
        });
    }

    return app.exec();
}

} // namespace TagOS::Runtime
