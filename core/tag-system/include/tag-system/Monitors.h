#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

#include "tag-common/Export.h"

namespace TagOS::System {

/// Live process table from /proc, sampled between refreshes.
///
/// CPU percentages need two samples, so the first refresh reports 0% and the
/// following ones are real deltas. The reader accepts an explicit /proc root so
/// tests can drive it with fixtures.
class TAGOS_EXPORT ProcessList : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY countChanged)
    Q_PROPERTY(int processCount READ processCount NOTIFY countChanged)
    Q_PROPERTY(int threadCount READ threadCount NOTIFY countChanged)

public:
    enum Roles {
        PidRole = Qt::UserRole + 1,
        NameRole,
        CpuRole,
        CpuTextRole,
        MemoryRole,
        MemoryTextRole,
        StateRole,
        CommandRole,
    };

    explicit ProcessList(QObject *parent = nullptr, const QString &procRoot = QStringLiteral("/proc"));

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool available() const { return m_available; }
    int count() const { return m_rows.size(); }
    int processCount() const { return m_processCount; }
    int threadCount() const { return m_threadCount; }
    QString summary() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantMap at(int row) const;
    Q_INVOKABLE bool kill(int pid);

signals:
    void availableChanged();
    void countChanged();
    void refreshFailed(const QString &reason);

private:
    struct Row
    {
        int pid = 0;
        QString name;
        QString command;
        QString state;
        double cpuPercent = 0.0;
        qint64 rssBytes = 0;
        double memoryPercent = 0.0;
        quint64 cpuTicks = 0;
    };

    bool readCpuTotal(quint64 *total) const;
    bool readProcess(int pid, Row *row, quint64 *totalTicks, quint64 *rssPages) const;
    qint64 totalMemoryBytes() const;

    QString m_root;
    bool m_available = false;
    QList<Row> m_rows;
    QHash<int, quint64> m_previousTicks;
    quint64 m_previousTotal = 0;
    int m_processCount = 0;
    int m_threadCount = 0;
    qint64 m_memoryTotal = 0;
    int m_cpuCount = 1;
};

/// Aggregate network throughput from /proc/net/dev.
class TAGOS_EXPORT NetworkSampler : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY statsChanged)
    Q_PROPERTY(QString interfaceName READ interfaceName NOTIFY statsChanged)
    Q_PROPERTY(double rxBytesPerSecond READ rxBytesPerSecond NOTIFY statsChanged)
    Q_PROPERTY(double txBytesPerSecond READ txBytesPerSecond NOTIFY statsChanged)
    Q_PROPERTY(QString rxRateText READ rxRateText NOTIFY statsChanged)
    Q_PROPERTY(QString txRateText READ txRateText NOTIFY statsChanged)
    Q_PROPERTY(QString rxTotalText READ rxTotalText NOTIFY statsChanged)
    Q_PROPERTY(QString txTotalText READ txTotalText NOTIFY statsChanged)

public:
    explicit NetworkSampler(QObject *parent = nullptr, const QString &procRoot = QStringLiteral("/proc"));

    bool available() const { return m_available; }
    QString interfaceName() const { return m_interface; }
    double rxBytesPerSecond() const { return m_rxRate; }
    double txBytesPerSecond() const { return m_txRate; }
    QString rxRateText() const;
    QString txRateText() const;
    QString rxTotalText() const;
    QString txTotalText() const;

    Q_INVOKABLE void refresh();

signals:
    void statsChanged();

private:
    QString m_root;
    bool m_available = false;
    QString m_interface;
    quint64 m_rxBytes = 0;
    quint64 m_txBytes = 0;
    quint64 m_previousRx = 0;
    quint64 m_previousTx = 0;
    double m_rxRate = 0.0;
    double m_txRate = 0.0;
    QElapsedTimer m_clock;
    bool m_hasSample = false;
};

/// Highest thermal zone temperature (Linux hwmon/thermal).
class TAGOS_EXPORT ThermalReader : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(double celsius READ celsius NOTIFY changed)
    Q_PROPERTY(QString label READ label NOTIFY changed)
    Q_PROPERTY(QString text READ text NOTIFY changed)

public:
    explicit ThermalReader(QObject *parent = nullptr,
                           const QString &thermalRoot = QStringLiteral("/sys/class/thermal"));

    bool available() const { return m_available; }
    double celsius() const { return m_celsius; }
    QString label() const { return m_label; }
    QString text() const;

    Q_INVOKABLE void refresh();

signals:
    void changed();

private:
    QString m_root;
    bool m_available = false;
    double m_celsius = 0.0;
    QString m_label;
};

} // namespace TagOS::System
