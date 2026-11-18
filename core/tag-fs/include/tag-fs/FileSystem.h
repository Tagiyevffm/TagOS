#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

#include "tag-common/Export.h"

namespace TagOS::Files {

/// One directory entry as shown by TagFiles and TagLaunch.
struct TAGOS_EXPORT FileEntry
{
    QString name;
    QString path;
    QString suffix;
    QString mimeType;
    QString iconName;
    qint64 size = 0;
    QDateTime modified;
    bool isDir = false;
    bool isSymlink = false;
    bool hidden = false;
    bool readable = true;
};

/// Human-readable helpers shared by every TagOS file surface.
namespace PathText {
TAGOS_EXPORT QString formatBytes(qint64 bytes);
TAGOS_EXPORT QString homePath();
TAGOS_EXPORT QString parentOf(const QString &path);
TAGOS_EXPORT QString join(const QString &dir, const QString &name);
TAGOS_EXPORT QString display(const QString &path);
TAGOS_EXPORT bool isRoot(const QString &path);
/// TagOS icon name for a file (kept in sync with themes/icons).
TAGOS_EXPORT QString iconFor(const QString &path, bool isDir, const QString &mimeType);
} // namespace PathText

/// QML-facing wrapper around the path helpers (a C++ namespace is not a QML type).
class TAGOS_EXPORT PathUtils : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit PathUtils(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString formatBytes(qint64 bytes) const { return PathText::formatBytes(bytes); }
    Q_INVOKABLE QString homePath() const { return PathText::homePath(); }
    Q_INVOKABLE QString parentOf(const QString &path) const { return PathText::parentOf(path); }
    Q_INVOKABLE QString join(const QString &dir, const QString &name) const { return PathText::join(dir, name); }
    Q_INVOKABLE QString display(const QString &path) const { return PathText::display(path); }
    Q_INVOKABLE bool isRoot(const QString &path) const { return PathText::isRoot(path); }
};

/// Directory listing model. Stat-only, no watcher: TagFiles refreshes on demand
/// and after each mutation, which keeps the cost per directory predictable.
class TAGOS_EXPORT DirModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString parentPath READ parentPath NOTIFY pathChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(QString sortBy READ sortBy WRITE setSortBy NOTIFY sortByChanged)
    Q_PROPERTY(bool foldersFirst READ foldersFirst WRITE setFoldersFirst NOTIFY foldersFirstChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        IsDirRole,
        SizeRole,
        SizeTextRole,
        ModifiedRole,
        ModifiedTextRole,
        HiddenRole,
        IconRole,
        MimeRole,
        SuffixRole,
    };

    explicit DirModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString path() const { return m_path; }
    QString parentPath() const;
    int count() const { return m_entries.size(); }
    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool show);
    QString sortBy() const { return m_sortBy; }
    void setSortBy(const QString &key);
    bool foldersFirst() const { return m_foldersFirst; }
    void setFoldersFirst(bool value);
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE bool setPath(const QString &path);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantMap at(int row) const;
    Q_INVOKABLE QVariantList all() const;
    Q_INVOKABLE int indexOf(const QString &path) const;

signals:
    void pathChanged();
    void countChanged();
    void showHiddenChanged();
    void sortByChanged();
    void foldersFirstChanged();
    void lastErrorChanged();

private:
    void rebuild();
    static FileEntry describe(const QString &path);

    QString m_path;
    QList<FileEntry> m_entries;
    bool m_showHidden = false;
    bool m_foldersFirst = true;
    QString m_sortBy = QStringLiteral("name");
    QString m_lastError;
};

/// File mutations used by TagFiles. Every operation reports a translated result
/// and never touches anything outside the paths it was given.
class TAGOS_EXPORT FileOps : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit FileOps(QObject *parent = nullptr);

    Q_INVOKABLE bool copy(const QStringList &sources, const QString &destinationDir);
    Q_INVOKABLE bool move(const QStringList &sources, const QString &destinationDir);
    Q_INVOKABLE bool rename(const QString &path, const QString &newName);
    Q_INVOKABLE bool makeDir(const QString &parentDir, const QString &name);
    /// Moves to the freedesktop trash via gio; refuses when gio is unavailable.
    Q_INVOKABLE bool moveToTrash(const QStringList &paths);
    /// Permanent delete. The UI must confirm before calling this.
    Q_INVOKABLE bool removePermanently(const QStringList &paths);

    Q_INVOKABLE QString lastError() const { return m_lastError; }
    Q_INVOKABLE bool lastOperationSucceeded() const { return m_ok; }
    Q_INVOKABLE QString copyNameFor(const QString &destinationDir, const QString &name) const;
    /// File name without its extension, for the rename dialog.
    Q_INVOKABLE QString baseName(const QString &path) const;

signals:
    void finished(const QString &operation, bool ok, const QString &message);

private:
    bool fail(const QString &message);
    bool succeed(const QString &message);
    static bool copyRecursively(const QString &source, const QString &target, QString *error);
    static bool removeRecursively(const QString &path, QString *error);

    bool m_ok = true;
    QString m_lastError;
};

/// Bounded recursive file search used by TagLaunch and TagFiles.
class TAGOS_EXPORT FileSearch : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)

public:
    explicit FileSearch(QObject *parent = nullptr);

    QVariantList results() const { return m_results; }
    bool searching() const { return m_searching; }

    /// Searches `root` (recursively) for names containing `query`.
    Q_INVOKABLE void start(const QString &root, const QString &query, int limit = 40);
    Q_INVOKABLE void clear();

signals:
    void resultsChanged();
    void searchingChanged();

private:
    QVariantList m_results;
    bool m_searching = false;
};

/// Preview payload for the TagFiles info pane.
class TAGOS_EXPORT Preview : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Preview(QObject *parent = nullptr);

    /// kind: "image", "text", "binary", "dir", "missing".
    Q_INVOKABLE QVariantMap describe(const QString &path) const;
};

} // namespace TagOS::Files
