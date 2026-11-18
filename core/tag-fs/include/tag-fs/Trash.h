#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

#include "tag-common/Export.h"

namespace TagOS::Files {

/// One entry in the trash.
struct TAGOS_EXPORT TrashEntry
{
    QString trashedPath;    ///< file inside <trash>/files
    QString infoPath;       ///< matching .trashinfo file
    QString originalPath;   ///< decoded Path= from the .trashinfo
    QDateTime deletedAt;
    qint64 size = 0;
    bool isDir = false;
};

/// FreeDesktop.org Trash specification implementation.
///
/// TagOS does not delegate deletion to `gio`: the spec is small enough to
/// implement correctly, and owning it keeps behaviour identical on every system
/// (including systems without glib) and testable without a Linux kernel.
///
/// Layout per the specification:
///   $XDG_DATA_HOME/Trash/{files,info}          — home trash
///   <mount>/.Trash-$uid/{files,info}           — trash on another filesystem
///
/// Every move is a rename on the same filesystem; cross-device moves fall back
/// to a copy that preserves symlinks as symlinks (never copying their targets).
class TAGOS_EXPORT TrashService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QString filesPath READ filesPath CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    /// `dataHome` overrides $XDG_DATA_HOME (tests pass a temporary directory).
    explicit TrashService(QObject *parent = nullptr, const QString &dataHome = QString());

    int count() const;
    QString filesPath() const;
    QString lastError() const { return m_lastError; }

    /// Moves each existing path into the correct trash for its filesystem.
    /// Returns false (and sets lastError) on the first failure; already moved
    /// entries stay moved and are listed afterwards.
    Q_INVOKABLE bool moveToTrash(const QStringList &paths);

    /// Entries, newest first.
    Q_INVOKABLE QVariantList list() const;

    /// Moves a trashed entry back to its original location.
    Q_INVOKABLE bool restore(const QString &trashedPath);

    /// Permanently deletes everything in the trash. The UI must confirm first.
    Q_INVOKABLE bool empty();

    /// Re-reads the info directory (cheap; called after any mutation).
    Q_INVOKABLE void refresh();

    /// Percent-encoding used by the spec for the Path= key (exposed for tests).
    Q_INVOKABLE static QString encodePath(const QString &path);
    Q_INVOKABLE static QString decodePath(const QString &encoded);

signals:
    void changed();
    void lastErrorChanged();
    void finished(const QString &operation, bool ok, const QString &message);

private:
    bool fail(const QString &message);
    bool succeed(const QString &message);
    QString homeTrashDir() const;
    QString trashDirFor(const QString &path, QString *mountPoint) const;
    QString uniqueName(const QString &dir, const QString &name) const;
    bool moveEntry(const QString &source, const QString &filesDir, const QString &infoDir, QString *error);
    static bool moveAcrossDevices(const QString &source, const QString &target, QString *error);
    QString m_dataHome;
    QString m_lastError;
    int m_count = 0;
};

} // namespace TagOS::Files
