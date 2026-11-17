#pragma once

#include <QList>
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QVariantList>
#include <QString>

#include <memory>

#include "tagflow/Backend.h"
#include "tagflow/FlowExport.h"

namespace TagOS::Flow {

/// QML-facing live system metrics client.
///
/// The shell and every TagOS app bind to one of these instead of reading
/// procfs themselves, so a future transport change never touches UI code.
class TAGOS_FLOW_EXPORT FlowClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool valid READ isValid NOTIFY snapshotChanged)
    Q_PROPERTY(QString transport READ transportId NOTIFY transportChanged)
    Q_PROPERTY(QString transportLabel READ transportLabel NOTIFY transportChanged)
    Q_PROPERTY(int pollIntervalMs READ pollIntervalMs WRITE setPollIntervalMs NOTIFY pollIntervalMsChanged)

    Q_PROPERTY(double cpuPercent READ cpuPercent NOTIFY snapshotChanged)
    Q_PROPERTY(QString cpuPercentText READ cpuPercentText NOTIFY snapshotChanged)
    Q_PROPERTY(int cpuCount READ cpuCount NOTIFY snapshotChanged)
    Q_PROPERTY(QString loadText READ loadText NOTIFY snapshotChanged)

    Q_PROPERTY(double memoryUsedPercent READ memoryUsedPercent NOTIFY snapshotChanged)
    Q_PROPERTY(QString memoryText READ memoryText NOTIFY snapshotChanged)
    Q_PROPERTY(double swapUsedPercent READ swapUsedPercent NOTIFY snapshotChanged)

    Q_PROPERTY(double diskUsedPercent READ diskUsedPercent NOTIFY snapshotChanged)
    Q_PROPERTY(QString diskText READ diskText NOTIFY snapshotChanged)

    Q_PROPERTY(qint64 uptimeSeconds READ uptimeSeconds NOTIFY snapshotChanged)
    Q_PROPERTY(QString uptimeText READ uptimeText NOTIFY snapshotChanged)

    Q_PROPERTY(QString hostName READ hostName NOTIFY snapshotChanged)
    Q_PROPERTY(QString distroName READ distroName NOTIFY snapshotChanged)
    Q_PROPERTY(QString kernelVersion READ kernelVersion NOTIFY snapshotChanged)
    Q_PROPERTY(QString tagosVersion READ tagosVersion CONSTANT)

    /// Recent CPU percentages (oldest first), for sparklines and charts.
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY historyChanged)
    Q_PROPERTY(QVariantList memoryHistory READ memoryHistory NOTIFY historyChanged)

public:
    explicit FlowClient(QObject *parent = nullptr);
    ~FlowClient() override;

    bool isValid() const { return m_valid; }
    QString transportId() const;
    QString transportLabel() const;
    int pollIntervalMs() const { return m_pollIntervalMs; }
    void setPollIntervalMs(int intervalMs);

    double cpuPercent() const { return m_snapshot.cpuPercent; }
    QString cpuPercentText() const;
    int cpuCount() const { return m_snapshot.cpuCount; }
    QString loadText() const;

    double memoryUsedPercent() const { return m_snapshot.memory.usedPercent(); }
    QString memoryText() const;
    double swapUsedPercent() const;

    double diskUsedPercent() const { return m_snapshot.disk.usedPercent(); }
    QString diskText() const;

    qint64 uptimeSeconds() const { return m_snapshot.uptimeSeconds; }
    QString uptimeText() const;

    QString hostName() const { return m_snapshot.hostName; }
    QString distroName() const { return m_snapshot.distroName; }
    QString kernelVersion() const { return m_snapshot.kernelVersion; }
    QString tagosVersion() const;

    QVariantList cpuHistory() const { return m_cpuHistory; }
    QVariantList memoryHistory() const { return m_memoryHistory; }

    /// Exposed for tests: the transport chosen at construction time.
    Transport transportEnum() const;

public slots:
    /// Sample immediately; used by "open the panel" style UI actions.
    void refresh();
    void clearHistory();

signals:
    void snapshotChanged();
    void transportChanged();
    void pollIntervalMsChanged();
    void historyChanged();
    void errorOccurred(const QString &message);

private:
    void restartTimer();
    void applySnapshot(const TagOS::System::Snapshot &snapshot);
    void switchToLocal(const QString &reason);
    void appendHistory(double cpu, double memory);

    std::unique_ptr<Backend> m_backend;
    TagOS::System::Snapshot m_snapshot;
    QList<double> m_cpuHistoryValues;
    QList<double> m_memoryHistoryValues;
    QVariantList m_cpuHistory;
    QVariantList m_memoryHistory;
    int m_pollIntervalMs = 1000;
    int m_timerId = 0;
    int m_consecutiveFailures = 0;
    bool m_valid = false;
    bool m_fellBackToLocal = false;
    bool m_started = false;

protected:
    void timerEvent(QTimerEvent *event) override;
};

} // namespace TagOS::Flow
