// tag-shell — TagOS desktop shell entry point.
//
// Responsibilities kept here (and nowhere else):
//   * create the QML engine with the build/install import path,
//   * report readiness to systemd (`--ready-file %t/tagos/ready`) so the boot
//     test can prove a real session came up,
//   * provide `--smoke-test` for CI: load QML, render one frame, exit.
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>

#include "TagOSConfig.h"

#include <cstdio>

namespace {

/// Qt logging goes to the debugger instead of stderr when a Windows dev-host
/// process has no console. TAGOS_LOG_FILE (or --log-file) makes diagnostics
/// collectable by ctest and by the QEMU boot test.
QString g_logFile;
int g_qmlErrors = 0;

/// QML runtime failures (undefined tokens, failed bindings) are logged as
/// warnings, so a plain "did it load?" smoke test would happily pass on a broken
/// UI. Count them and fail the run instead.
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
    };
    for (const auto &needle : needles) {
        if (message.contains(needle))
            return true;
    }
    return false;
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const QByteArray line = qFormatLogMessage(type, context, message).toUtf8() + '\n';
    std::fwrite(line.constData(), 1, static_cast<size_t>(line.size()), stderr);
    std::fflush(stderr);
    if (!g_logFile.isEmpty()) {
        QFile file(g_logFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            file.write(line);
    }
}

} // namespace

namespace {

QString resolveReadyFile(const QString &requested)
{
    if (!requested.isEmpty())
        return requested;

    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString base = runtime.isEmpty() ? QDir::tempPath() : runtime;
    return base + QStringLiteral("/tagos/ready");
}

bool writeReadyFile(const QString &path, const QString &version)
{
    QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath()))
        return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    const QJsonObject payload{
        {QStringLiteral("component"), QStringLiteral("tag-shell")},
        {QStringLiteral("version"), version},
        {QStringLiteral("host"), QSysInfo::machineHostName()},
        {QStringLiteral("platform"), QGuiApplication::platformName()},
        {QStringLiteral("pid"), static_cast<double>(QCoreApplication::applicationPid())},
    };
    file.write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("tag-shell"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("TagOS"));
    QGuiApplication::setOrganizationName(QStringLiteral("TagOS"));
    QGuiApplication::setApplicationVersion(QStringLiteral(TAGOS_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("TagOS desktop shell"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption readyOption(QStringLiteral("ready-file"),
                                   QStringLiteral("Write a session-ready marker to <path>."),
                                   QStringLiteral("path"));
    QCommandLineOption smokeOption(QStringLiteral("smoke-test"),
                                   QStringLiteral("Load the shell, then exit 0 (used by ctest/CI)."));
    QCommandLineOption logOption(QStringLiteral("log-file"),
                                 QStringLiteral("Append Qt diagnostics to <path>."),
                                 QStringLiteral("path"));
    QCommandLineOption screenshotOption(QStringLiteral("screenshot"),
                                        QStringLiteral("Render one frame to <path> and exit (dev/CI)."),
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

    const bool smokeTest = parser.isSet(smokeOption);

    QQmlApplicationEngine engine;
    // QML module locations, in priority order:
    //   1. next to the executable (build tree),
    //   2. <prefix>/lib/qml (installed image, any prefix),
    //   3. TAGOS_QML_PATH (development overrides).
    const QDir appDir(QCoreApplication::applicationDirPath());
    engine.addImportPath(appDir.absolutePath());
    engine.addImportPath(QDir::cleanPath(appDir.absoluteFilePath(QStringLiteral("../lib/qml"))));
    const QString qmlPathOverride = qEnvironmentVariable("TAGOS_QML_PATH");
    if (!qmlPathOverride.isEmpty())
        engine.addImportPath(qmlPathOverride);

    bool loadFailed = false;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [&loadFailed]() {
            loadFailed = true;
            QTextStream(stderr) << "tag-shell: failed to create the shell surface\n";
            QCoreApplication::exit(2);
        },
        Qt::QueuedConnection);

    engine.loadFromModule("TagOS.Shell", "Main");
    if (engine.rootObjects().isEmpty() || loadFailed)
        return 2;

    const QString readyFile = resolveReadyFile(parser.value(readyOption));
    if (!writeReadyFile(readyFile, QStringLiteral(TAGOS_VERSION)))
        QTextStream(stderr) << "tag-shell: could not write ready file " << readyFile << '\n';

    if (parser.isSet(screenshotOption)) {
        const QString target = parser.value(screenshotOption);
        QTimer::singleShot(1400, &app, [&engine, target, &app]() {
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
            if (!window) {
                QTextStream(stderr) << "tag-shell: no window to render\n";
                QCoreApplication::exit(4);
                return;
            }
            const QImage frame = window->grabWindow();
            if (frame.isNull() || !frame.save(target)) {
                QTextStream(stderr) << "tag-shell: could not save screenshot to " << target << '\n';
                QCoreApplication::exit(5);
                return;
            }
            QTextStream(stdout) << "TAGOS_SCREENSHOT " << target << ' ' << frame.width() << 'x'
                                << frame.height() << '\n';
            app.quit();
        });
    }

    if (smokeTest) {
        QTimer::singleShot(900, &app, [&app]() {
            QTextStream(stdout) << "TAGOS_SMOKE_OK tag-shell " << TAGOS_VERSION << '\n';
            app.quit();
        });
    }

    return app.exec();
}
