// tagos-metrics — print one system snapshot as JSON or as a human summary.
//
// Used by the QEMU boot test (`tagos-metrics --json` + jq/grep on the serial
// console), by tag-flowd for its `--once` mode and by developers.
#include "tag-system/Format.h"
#include "tag-system/SystemProbe.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>
#include <QThread>

using namespace TagOS::System;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("tagos-metrics"));
    QCoreApplication::setApplicationVersion(tagosVersion());

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Print a TagOS system snapshot"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption jsonOption({QStringLiteral("j"), QStringLiteral("json")},
                                  QStringLiteral("Emit JSON instead of a human summary."));
    QCommandLineOption watchOption({QStringLiteral("w"), QStringLiteral("watch")},
                                   QStringLiteral("Sample continuously."));
    QCommandLineOption intervalOption({QStringLiteral("i"), QStringLiteral("interval")},
                                      QStringLiteral("Sampling interval in ms (default 1000)."),
                                      QStringLiteral("ms"), QStringLiteral("1000"));
    parser.addOption(jsonOption);
    parser.addOption(watchOption);
    parser.addOption(intervalOption);
    parser.process(app);

    const bool emitJson = parser.isSet(jsonOption);
    const bool watch = parser.isSet(watchOption);
    bool intervalOk = false;
    const int intervalMs = qMax(100, parser.value(intervalOption).toInt(&intervalOk));
    const int effectiveInterval = intervalOk ? intervalMs : 1000;

    auto printSnapshot = [](const Snapshot &snapshot, bool json) {
        QTextStream out(stdout);
        if (json) {
            out << QString::fromUtf8(QJsonDocument(toJson(snapshot)).toJson(QJsonDocument::Compact)) << Qt::endl;
            out.flush();
            return;
        }
        out << "TagOS " << tagosVersion() << "  ·  " << (snapshot.distroName.isEmpty() ? platformDisplayName() : snapshot.distroName)
            << Qt::endl;
        out << "host      : " << snapshot.hostName << Qt::endl;
        out << "kernel    : " << snapshot.kernelVersion << " (" << snapshot.arch << ')' << Qt::endl;
        out << "cpu       : " << formatPercent(snapshot.cpuPercent) << " of " << snapshot.cpuCount << " threads"
            << Qt::endl;
        out << "memory    : " << formatBytes(snapshot.memory.usedBytes()) << " / "
            << formatBytes(snapshot.memory.totalBytes) << " (" << formatPercent(snapshot.memory.usedPercent(), 1) << ')'
            << Qt::endl;
        out << "disk      : " << formatBytes(snapshot.disk.usedBytes()) << " / "
            << formatBytes(snapshot.disk.totalBytes) << " (" << formatPercent(snapshot.disk.usedPercent(), 1) << ')'
            << Qt::endl;
        out << "uptime    : " << formatUptime(snapshot.uptimeSeconds) << Qt::endl;
        out << "load      : " << formatLoad(snapshot.load) << Qt::endl;
        out.flush();
    };

    // The first sample only primes the CPU delta, so a single shot stays honest.
    Snapshot snapshot = sampleSystem();

    if (!watch) {
        if (snapshot.cpuPercent < 0.0) {
            QThread::msleep(120);
            snapshot = sampleSystem();
        }
        printSnapshot(snapshot, emitJson);
        return 0;
    }

    for (;;) {
        QThread::msleep(static_cast<unsigned long>(effectiveInterval));
        snapshot = sampleSystem();
        printSnapshot(snapshot, emitJson);
    }
}
