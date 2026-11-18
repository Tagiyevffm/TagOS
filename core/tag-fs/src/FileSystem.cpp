#include "tag-fs/FileSystem.h"
#include "tag-fs/Trash.h"

#include <algorithm>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QStandardPaths>

namespace TagOS::Files {
namespace {

QString elideHome(const QString &path)
{
    const QString home = PathText::homePath();
    if (!home.isEmpty() && path == home)
        return QStringLiteral("~");
    if (!home.isEmpty() && path.startsWith(home + QLatin1Char('/')))
        return QStringLiteral("~") + path.mid(home.size());
    return path;
}

} // namespace

namespace PathText {

QString formatBytes(qint64 bytes)
{
    if (bytes < 0)
        return QStringLiteral("--");
    static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    if (unit == 0)
        return QStringLiteral("%1 B").arg(bytes);
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', value < 10 ? 1 : 0)).arg(QLatin1String(units[unit]));
}

QString homePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

QString parentOf(const QString &path)
{
    const QString clean = QDir::cleanPath(path);
    if (clean.isEmpty() || clean == QLatin1String("/"))
        return QString();
    const int slash = clean.lastIndexOf(QLatin1Char('/'));
    if (slash == 0)
        return QStringLiteral("/");
    if (slash < 0)
        return QString();
    return clean.left(slash);
}

QString join(const QString &dir, const QString &name)
{
    if (dir.endsWith(QLatin1Char('/')))
        return dir + name;
    return dir + QLatin1Char('/') + name;
}

QString display(const QString &path)
{
    return elideHome(QDir::cleanPath(path));
}

bool isRoot(const QString &path)
{
    return QDir::cleanPath(path) == QLatin1String("/");
}

QString iconFor(const QString &path, bool isDir, const QString &mimeType)
{
    if (isDir)
        return QStringLiteral("folder");

    if (mimeType.startsWith(QLatin1String("image/")))
        return QStringLiteral("image");
    if (mimeType.startsWith(QLatin1String("text/")) || mimeType.contains(QLatin1String("json"))
        || mimeType.contains(QLatin1String("xml"))) {
        return QStringLiteral("file");
    }
    if (mimeType.contains(QLatin1String("shellscript")) || mimeType.contains(QLatin1String("executable")))
        return QStringLiteral("terminal");

    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("sh") || suffix == QLatin1String("bash"))
        return QStringLiteral("terminal");
    if (suffix == QLatin1String("zip") || suffix == QLatin1String("tar") || suffix == QLatin1String("gz")
        || suffix == QLatin1String("xz") || suffix == QLatin1String("zst")) {
        return QStringLiteral("package");
    }
    if (suffix == QLatin1String("iso"))
        return QStringLiteral("disk");
    return QStringLiteral("file");
}

} // namespace PathText

// ---------------------------------------------------------------------------
// DirModel
// ---------------------------------------------------------------------------

DirModel::DirModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_path(PathText::homePath())
{
    rebuild();
}

int DirModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant DirModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const FileEntry &entry = m_entries.at(index.row());

    switch (role) {
    case NameRole:
        return entry.name;
    case PathRole:
        return entry.path;
    case IsDirRole:
        return entry.isDir;
    case SizeRole:
        return entry.size;
    case SizeTextRole:
        return entry.isDir ? QString() : PathText::formatBytes(entry.size);
    case ModifiedRole:
        return entry.modified;
    case ModifiedTextRole:
        return entry.modified.isValid() ? entry.modified.toString(QStringLiteral("dd.MM.yyyy HH:mm")) : QString();
    case HiddenRole:
        return entry.hidden;
    case IconRole:
        return entry.iconName;
    case MimeRole:
        return entry.mimeType;
    case SuffixRole:
        return entry.suffix;
    default:
        return {};
    }
}

QHash<int, QByteArray> DirModel::roleNames() const
{
    return {
        {NameRole, "name"},           {PathRole, "filePath"},
        {IsDirRole, "isDir"},         {SizeRole, "size"},
        {SizeTextRole, "sizeText"},   {ModifiedRole, "modified"},
        {ModifiedTextRole, "modifiedText"}, {HiddenRole, "hidden"},
        {IconRole, "iconName"},       {MimeRole, "mimeType"},
        {SuffixRole, "suffix"},
    };
}

QString DirModel::parentPath() const
{
    return PathText::parentOf(m_path);
}

void DirModel::setShowHidden(bool show)
{
    if (m_showHidden == show)
        return;
    m_showHidden = show;
    emit showHiddenChanged();
    rebuild();
}

void DirModel::setSortBy(const QString &key)
{
    static const QStringList allowed = {QStringLiteral("name"), QStringLiteral("size"),
                                        QStringLiteral("modified"), QStringLiteral("type")};
    if (!allowed.contains(key) || m_sortBy == key)
        return;
    m_sortBy = key;
    emit sortByChanged();
    rebuild();
}

void DirModel::setFoldersFirst(bool value)
{
    if (m_foldersFirst == value)
        return;
    m_foldersFirst = value;
    emit foldersFirstChanged();
    rebuild();
}

bool DirModel::setPath(const QString &path)
{
    const QString clean = QDir::cleanPath(path.isEmpty() ? PathText::homePath() : path);
    const QFileInfo info(clean);
    if (!info.exists() || !info.isDir()) {
        m_lastError = tr("Klasör bulunamadı: %1").arg(clean);
        emit lastErrorChanged();
        return false;
    }
    if (!info.isReadable()) {
        m_lastError = tr("Bu klasöre erişim yok: %1").arg(clean);
        emit lastErrorChanged();
        return false;
    }

    m_lastError.clear();
    emit lastErrorChanged();
    if (m_path == clean) {
        refresh();
        return true;
    }
    m_path = clean;
    emit pathChanged();
    rebuild();
    return true;
}

void DirModel::refresh()
{
    rebuild();
}

FileEntry DirModel::describe(const QString &path)
{
    static QMimeDatabase mimeDb;
    const QFileInfo info(path);
    FileEntry entry;
    entry.name = info.fileName();
    if (entry.name.isEmpty())
        entry.name = info.absoluteFilePath(); // "/" has no fileName
    entry.path = info.absoluteFilePath();
    entry.isDir = info.isDir();
    entry.isSymlink = info.isSymLink();
    entry.hidden = info.fileName().startsWith(QLatin1Char('.')) && info.fileName() != QLatin1String(".");
    entry.readable = info.isReadable();
    entry.size = entry.isDir ? 0 : info.size();
    entry.modified = info.lastModified();
    entry.suffix = info.suffix().toLower();
    if (entry.isDir) {
        entry.mimeType = QStringLiteral("inode/directory");
    } else {
        const QMimeType mime = mimeDb.mimeTypeForFile(info, QMimeDatabase::MatchExtension);
        entry.mimeType = mime.name();
    }
    entry.iconName = PathText::iconFor(entry.path, entry.isDir, entry.mimeType);
    return entry;
}

void DirModel::rebuild()
{
    const QFileInfo dirInfo(m_path);
    QList<FileEntry> entries;

    if (!dirInfo.exists() || !dirInfo.isDir()) {
        beginResetModel();
        m_entries.clear();
        endResetModel();
        emit countChanged();
        m_lastError = tr("Klasör yok: %1").arg(m_path);
        emit lastErrorChanged();
        return;
    }

    const QDir dir(m_path);
    const QFileInfoList infos = dir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System | (m_showHidden ? QDir::Hidden : QDir::Files),
        QDir::NoSort);

    entries.reserve(infos.size());
    for (const QFileInfo &info : infos) {
        if (!m_showHidden && info.fileName().startsWith(QLatin1Char('.')))
            continue;
        entries.append(describe(info.absoluteFilePath()));
    }

    std::sort(entries.begin(), entries.end(), [this](const FileEntry &a, const FileEntry &b) {
        if (m_foldersFirst && a.isDir != b.isDir)
            return a.isDir;
        if (m_sortBy == QLatin1String("size"))
            return a.size > b.size;
        if (m_sortBy == QLatin1String("modified"))
            return a.modified > b.modified;
        if (m_sortBy == QLatin1String("type"))
            return a.suffix.localeAwareCompare(b.suffix) < 0;
        return a.name.localeAwareCompare(b.name) < 0;
    });

    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

QVariantMap DirModel::at(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    const FileEntry &entry = m_entries.at(row);
    return {
        {QStringLiteral("name"), entry.name},
        {QStringLiteral("filePath"), entry.path},
        {QStringLiteral("isDir"), entry.isDir},
        {QStringLiteral("size"), entry.size},
        {QStringLiteral("sizeText"), entry.isDir ? QString() : PathText::formatBytes(entry.size)},
        {QStringLiteral("modifiedText"),
         entry.modified.isValid() ? entry.modified.toString(QStringLiteral("dd.MM.yyyy HH:mm")) : QString()},
        {QStringLiteral("iconName"), entry.iconName},
        {QStringLiteral("mimeType"), entry.mimeType},
        {QStringLiteral("hidden"), entry.hidden},
    };
}

QVariantList DirModel::all() const
{
    QVariantList list;
    list.reserve(m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i)
        list.append(at(i));
    return list;
}

int DirModel::indexOf(const QString &path) const
{
    const QString clean = QDir::cleanPath(path);
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).path == clean)
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// FileOps
// ---------------------------------------------------------------------------

FileOps::FileOps(QObject *parent)
    : QObject(parent)
{
}

bool FileOps::fail(const QString &message)
{
    m_ok = false;
    m_lastError = message;
    emit finished(QStringLiteral("error"), false, message);
    return false;
}

bool FileOps::succeed(const QString &message)
{
    m_ok = true;
    m_lastError.clear();
    emit finished(QStringLiteral("ok"), true, message);
    return true;
}

bool FileOps::copyRecursively(const QString &source, const QString &target, QString *error)
{
    const QFileInfo info(source);
    if (info.isDir()) {
        if (!QDir().mkpath(target)) {
            *error = FileOps::tr("Klasör oluşturulamadı: %1").arg(target);
            return false;
        }
        const QFileInfoList children = QDir(source).entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QFileInfo &child : children) {
            if (!copyRecursively(child.absoluteFilePath(), PathText::join(target, child.fileName()), error))
                return false;
        }
        return true;
    }
    if (!QFile::copy(source, target)) {
        *error = FileOps::tr("Kopyalanamadı: %1").arg(source);
        return false;
    }
    return true;
}

bool FileOps::removeRecursively(const QString &path, QString *error)
{
    const QFileInfo info(path);
    if (info.isDir() && !info.isSymLink()) {
        const QFileInfoList children = QDir(path).entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QFileInfo &child : children) {
            if (!removeRecursively(child.absoluteFilePath(), error))
                return false;
        }
        if (!QDir().rmdir(path)) {
            *error = FileOps::tr("Klasör silinemedi: %1").arg(path);
            return false;
        }
        return true;
    }
    if (!QFile::remove(path)) {
        *error = FileOps::tr("Dosya silinemedi: %1").arg(path);
        return false;
    }
    return true;
}

QString FileOps::baseName(const QString &path) const
{
    const QFileInfo info(path);
    const QString name = info.fileName();
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    if (dot > 0)
        return name.left(dot);
    return name;
}

QString FileOps::copyNameFor(const QString &destinationDir, const QString &name) const
{
    QString candidate = name;
    QString base = name;
    QString suffix;
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    if (dot > 0) {
        base = name.left(dot);
        suffix = name.mid(dot);
    }
    int counter = 1;
    while (QFileInfo::exists(PathText::join(destinationDir, candidate))) {
        candidate = base + QStringLiteral(" (%1)").arg(counter++) + suffix;
    }
    return candidate;
}

bool FileOps::copy(const QStringList &sources, const QString &destinationDir)
{
    if (sources.isEmpty())
        return fail(tr("Kopyalanacak öğe yok"));
    const QFileInfo destInfo(destinationDir);
    if (!destInfo.isDir() || !destInfo.isWritable())
        return fail(tr("Hedef klasör yazılabilir değil: %1").arg(destinationDir));

    int copied = 0;
    for (const QString &source : sources) {
        const QFileInfo info(source);
        if (!info.exists())
            continue;
        const QString target = PathText::join(destinationDir, copyNameFor(destinationDir, info.fileName()));
        QString error;
        if (!copyRecursively(source, target, &error))
            return fail(error);
        ++copied;
    }
    return succeed(tr("%1 öğe kopyalandı").arg(copied));
}

bool FileOps::move(const QStringList &sources, const QString &destinationDir)
{
    if (sources.isEmpty())
        return fail(tr("Taşınacak öğe yok"));
    const QFileInfo destInfo(destinationDir);
    if (!destInfo.isDir() || !destInfo.isWritable())
        return fail(tr("Hedef klasör yazılabilir değil: %1").arg(destinationDir));

    int moved = 0;
    for (const QString &source : sources) {
        const QFileInfo info(source);
        if (!info.exists())
            continue;
        const QString target = PathText::join(destinationDir, copyNameFor(destinationDir, info.fileName()));
        if (QFile::rename(source, target)) {
            ++moved;
            continue;
        }
        // Cross-device: copy then delete.
        QString error;
        if (!copyRecursively(source, target, &error))
            return fail(error);
        if (!removeRecursively(source, &error))
            return fail(error);
        ++moved;
    }
    return succeed(tr("%1 öğe taşındı").arg(moved));
}

bool FileOps::rename(const QString &path, const QString &newName)
{
    const QFileInfo info(path);
    if (!info.exists())
        return fail(tr("Öğe bulunamadı: %1").arg(path));
    const QString trimmed = newName.trimmed();
    if (trimmed.isEmpty())
        return fail(tr("Yeni ad boş olamaz"));
    if (trimmed.contains(QLatin1Char('/')))
        return fail(tr("Ad eğik çizgi içeremez"));

    const QString target = PathText::join(info.absolutePath(), trimmed);
    if (QFileInfo::exists(target) && target != info.absoluteFilePath())
        return fail(tr("Bu ad zaten var: %1").arg(trimmed));
    if (!QFile::rename(info.absoluteFilePath(), target))
        return fail(tr("Yeniden adlandırılamadı: %1").arg(info.fileName()));
    return succeed(tr("Yeniden adlandırıldı: %1").arg(trimmed));
}

bool FileOps::makeDir(const QString &parentDir, const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return fail(tr("Klasör adı boş olamaz"));
    const QString target = PathText::join(parentDir, trimmed);
    if (QFileInfo::exists(target))
        return fail(tr("Bu ad zaten var: %1").arg(trimmed));
    if (!QDir().mkdir(target))
        return fail(tr("Klasör oluşturulamadı: %1").arg(trimmed));
    return succeed(tr("Klasör oluşturuldu: %1").arg(trimmed));
}

bool FileOps::moveToTrash(const QStringList &paths)
{
    if (paths.isEmpty())
        return fail(tr("Silinecek öğe yok"));

    // The FreeDesktop Trash specification is implemented in TagOS::Files::Trash,
    // so deletion never depends on an external `gio` binary being installed.
    TrashService trash;
    QObject::connect(&trash, &TrashService::finished, this, [this](const QString &, bool ok, const QString &message) {
        if (!ok)
            m_lastError = message;
    });
    if (!trash.moveToTrash(paths))
        return fail(trash.lastError());
    return succeed(tr("%1 öğe çöp kutusuna taşındı").arg(paths.size()));
}

bool FileOps::removePermanently(const QStringList &paths)
{
    if (paths.isEmpty())
        return fail(tr("Silinecek öğe yok"));

    for (const QString &path : paths) {
        QString error;
        if (!removeRecursively(path, &error))
            return fail(error);
    }
    return succeed(tr("%1 öğe kalıcı olarak silindi").arg(paths.size()));
}

// ---------------------------------------------------------------------------
// FileSearch
// ---------------------------------------------------------------------------

FileSearch::FileSearch(QObject *parent)
    : QObject(parent)
{
}

void FileSearch::clear()
{
    if (m_results.isEmpty())
        return;
    m_results.clear();
    emit resultsChanged();
}

void FileSearch::start(const QString &root, const QString &query, int limit)
{
    const QString needle = query.trimmed();
    if (needle.isEmpty() || root.isEmpty()) {
        clear();
        return;
    }

    m_searching = true;
    emit searchingChanged();
    m_results.clear();

    // Hidden directories are skipped: a launcher search should never walk
    // ~/.cache or a git object store.
    QDirIterator it(root,
                    QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable,
                    QDirIterator::Subdirectories);
    int visited = 0;
    while (it.hasNext() && m_results.size() < limit && visited < 20000) {
        const QString path = it.next();
        ++visited;
        const QString name = it.fileName();
        if (!name.contains(needle, Qt::CaseInsensitive))
            continue;
        const QFileInfo info = it.fileInfo();
        const bool isDir = info.isDir();
        QString mime = QStringLiteral("inode/directory");
        if (!isDir) {
            static QMimeDatabase db;
            mime = db.mimeTypeForFile(info, QMimeDatabase::MatchExtension).name();
        }
        m_results.append(QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("filePath"), info.absoluteFilePath()},
            {QStringLiteral("parent"), PathText::display(info.absolutePath())},
            {QStringLiteral("isDir"), isDir},
            {QStringLiteral("iconName"), PathText::iconFor(path, isDir, mime)},
        });
    }

    m_searching = false;
    emit resultsChanged();
    emit searchingChanged();
}

// ---------------------------------------------------------------------------
// Preview
// ---------------------------------------------------------------------------

Preview::Preview(QObject *parent)
    : QObject(parent)
{
}

QVariantMap Preview::describe(const QString &path) const
{
    const QFileInfo info(path);
    if (!info.exists())
        return {{QStringLiteral("kind"), QStringLiteral("missing")}};

    QVariantMap base{
        {QStringLiteral("name"), info.fileName()},
        {QStringLiteral("path"), info.absoluteFilePath()},
        {QStringLiteral("size"), PathText::formatBytes(info.size())},
        {QStringLiteral("modified"), info.lastModified().toString(QStringLiteral("dd.MM.yyyy HH:mm"))},
        {QStringLiteral("readable"), info.isReadable()},
        {QStringLiteral("writable"), info.isWritable()},
    };

    if (info.isDir()) {
        base.insert(QStringLiteral("kind"), QStringLiteral("dir"));
        base.insert(QStringLiteral("entries"), QDir(path).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).size());
        return base;
    }

    static QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(info, QMimeDatabase::MatchExtension);
    base.insert(QStringLiteral("mime"), mime.name());

    if (mime.name().startsWith(QLatin1String("image/"))) {
        QImageReader reader(path);
        const QSize size = reader.size();
        if (size.isValid()) {
            base.insert(QStringLiteral("kind"), QStringLiteral("image"));
            base.insert(QStringLiteral("url"), QUrl::fromLocalFile(path).toString());
            base.insert(QStringLiteral("width"), size.width());
            base.insert(QStringLiteral("height"), size.height());
            return base;
        }
    }

    if (mime.name().startsWith(QLatin1String("text/")) || mime.name().contains(QLatin1String("json"))
        || mime.name().contains(QLatin1String("xml")) || mime.name().contains(QLatin1String("shellscript"))) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray head = file.read(4096);
            base.insert(QStringLiteral("kind"), QStringLiteral("text"));
            base.insert(QStringLiteral("text"), QString::fromUtf8(head));
            base.insert(QStringLiteral("truncated"), file.size() > head.size());
            return base;
        }
    }

    base.insert(QStringLiteral("kind"), QStringLiteral("binary"));
    return base;
}

} // namespace TagOS::Files
