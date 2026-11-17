// tag-flowd — TagOS system metrics service (org.tagos.Flow on the session bus).
//
// Phase 0 scope: expose CPU/RAM/disk/load/uptime so the shell never reads
// procfs itself. Later phases add service discovery for tag-power, tag-update,
// tag-notify on the same bus with the same "degrade, never crash" contract.
#include "FlowService.h"

#include "tag-system/Metrics.h"
#include "tag-system/SystemProbe.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QJsonDocument>
#include <QTextStream>
#include <QTimer>
#include <QThread>

using namespace TagOS;

namespace {
constexpr auto kServiceName = "org.tagos.Flow";
constexpr auto kObjectPath = "/org/tagos/Flow";

int printOnce(bool json)
{
    // Two samples 150 ms apart so the CPU percentage is meaningful.
    System::sampleSystem();
    QThread::msleep(150);
    const System::Snapshot snapshot = System::sampleSystem();

    QTextStream out(stdout);
    if (json)
        out << QString::fromUtf8(QJsonDocument(System::toJson(snapshot)).toJson(QJsonDocument::Compact));
    else
        out << "cpu=" << snapshot.cpuPercent << " mem=" << snapshot.memory.usedPercent()
            << "% disk=" << snapshot.disk.usedPercent() << "% uptime=" << snapshot.uptimeSeconds;
    out << Qt::endl;
    return 0;
}
} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("tag-flowd"));
    QCoreApplication::setApplicationVersion(System::tagosVersion());

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("TagOS metrics service (org.tagos.Flow1)"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption onceOption(QStringLiteral("once"), QStringLiteral("Print one snapshot and exit."));
    QCommandLineOption jsonOption(QStringLiteral("json"), QStringLiteral("With --once: emit JSON."));
    QCommandLineOption intervalOption(QStringLiteral("interval"),
                                      QStringLiteral("Sampling interval in ms (default 1500)."),
                                      QStringLiteral("ms"),
                                      QStringLiteral("1500"));
    parser.addOption(onceOption);
    parser.addOption(jsonOption);
    parser.addOption(intervalOption);
    parser.process(app);

    if (parser.isSet(onceOption))
        return printOnce(parser.isSet(jsonOption));

    bool intervalOk = false;
    const int intervalMs = qMax(200, parser.value(intervalOption).toInt(&intervalOk));

    Flow::FlowService service;
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        QTextStream(stderr) << "tag-flowd: session bus unavailable, exporting in-process only\n";
        return 2;
    }

    if (!bus.registerService(QString::fromLatin1(kServiceName))) {
        QTextStream(stderr) << "tag-flowd: could not own " << kServiceName << ": "
                            << bus.lastError().message() << '\n';
        return 3;
    }

    if (!bus.registerObject(QString::fromLatin1(kObjectPath),
                            &service,
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties
                                | QDBusConnection::ExportAllSignals)) {
        QTextStream(stderr) << "tag-flowd: could not export " << kObjectPath << '\n';
        return 4;
    }

    QTimer timer;
    timer.setInterval(intervalMs);
    QObject::connect(&timer, &QTimer::timeout, &service, &Flow::FlowService::poll);
    timer.start();
    service.poll();

    return app.exec();
}
