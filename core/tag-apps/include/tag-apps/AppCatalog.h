#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

#include "tag-apps/DesktopEntry.h"
#include "tag-common/Export.h"

namespace TagOS::Apps {

class AppCatalogPrivate;

/// Scans the XDG application directories and exposes launchable applications to
/// QML (TagLaunch, the dock, TagSettings).
///
/// Entries are merged by id with the most specific data dir winning, `Hidden` /
/// `NoDisplay` entries are dropped, and `TryExec` is resolved against PATH so a
/// launcher never shows an application that cannot start.
class TAGOS_EXPORT AppCatalog : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool loaded READ isLoaded NOTIFY changed)
    Q_PROPERTY(QStringList searchPaths READ searchPaths CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit AppCatalog(QObject *parent = nullptr);
    ~AppCatalog() override;

    QVariantList entries() const;
    int count() const;
    bool isLoaded() const;
    QStringList searchPaths() const { return m_searchPaths; }
    QString lastError() const;

    /// Read-only access for C++ tests and tools.
    QList<DesktopEntry> allEntries() const { return m_entries; }

    Q_INVOKABLE void refresh();
    /// Ranked search over name/generic name/keywords/category/exec.
    Q_INVOKABLE QVariantList search(const QString &query, int limit = 10) const;
    Q_INVOKABLE QVariantMap entry(const QString &id) const;
    Q_INVOKABLE bool launch(const QString &id);
    Q_INVOKABLE bool launchCommand(const QString &command);
    /// Opens a file or folder with the session's default handler.
    Q_INVOKABLE bool launchFile(const QString &path);

signals:
    void changed();
    void lastErrorChanged();
    void launchRequested(const QString &id, const QString &command);
    void launchFailed(const QString &id, const QString &reason);

private:
    static QVariantMap toVariant(const DesktopEntry &entry);
    QString terminalCommand(const QString &execLine) const;

    QList<DesktopEntry> m_entries;
    QStringList m_searchPaths;
    QString m_lastError;
    bool m_loaded = false;
};

} // namespace TagOS::Apps
