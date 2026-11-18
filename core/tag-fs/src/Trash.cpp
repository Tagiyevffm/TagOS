#include "tag-fs/Trash.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace TagOS::Files {
namespace {

/// Filesystem device of a path, used to pick the right trash directory.
/// Returns 0 when unknown (Windows dev builds), which means "use the home trash".
quint64 deviceOf(const QString &path)
{
#ifdef Q_OS_UNIX
    struct stat info {};
    if (::stat(QFile::encodeName(path).constData(), &info) == 0)
        return static_cast<quint64>(info.st_dev);
#else
    Q_UNUSED(path)
#endif
    return 0;
}

/// Walks up until the device changes, so /home/user/dir on a separate mount
/// gets its own trash instead of failing with EXDEV.
QString mountPointOf(const QString &path)
{
    const quint64 device = deviceOf(path);
    if (device == 0)
        return QString();

    QString current = QDir::cleanPath(path);
    if (!QFileInfo(current).isDir())
        current = QFileInfo(current).absolutePath();

    while (current != QLatin1String("/") && !current.isEmpty()) {
        const QString parent = QFileInfo(current).absolutePath();
        if (parent == current)
            break;
        if (deviceOf(parent) != device)
            break;
        current = parent;
    }
    return current;
}

QString currentUserName()
{
    const QByteArray user = qgetenv("USER");
    if (!user.isEmpty())
        return QString::fromLocal8Bit(user);
    return QString::number(
#ifdef Q_OS_UNIX
        static_cast<long>(::getuid())
#else
        0
#endif
    );
}

QString deletionStamp(const QDateTime &when)
{
    // DeletionDate=YYYY-MM-DDThh:mm:ss (local time, no timezone suffix).
    return when.toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
}

} // namespace

TrashService::TrashService(QObject *parent, const QString &dataHome)
    : QObject(parent)
    , m_dataHome(dataHome)
{
    refresh();
}

QString TrashService::filesPath() const
{
    return homeTrashDir() + QStringLiteral("/files");
}

QString TrashService::homeTrashDir() const
{
    // $XDG_DATA_HOME/Trash, defaulting to ~/.local/share/Trash per the spec.
    const QString dataHome = m_dataHome.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        : m_dataHome;
    if (!dataHome.isEmpty())
        return dataHome + QStringLiteral("/Trash");
    return QDir::homePath() + QStringLiteral("/.local/share/Trash");
}

QString TrashService::trashDirFor(const QString &path, QString *mountPoint) const
{
    const QString homeTrash = homeTrashDir();
    const QString fileDir = QFileInfo(path).absolutePath();

    const quint64 homeDevice = deviceOf(homeTrash);
    const quint64 fileDevice = deviceOf(fileDir);

    // Unknown device (non-POSIX) or same filesystem: the home trash is correct.
    if (fileDevice == 0 || homeDevice == 0 || fileDevice == homeDevice) {
        if (mountPoint)
            mountPoint->clear();
        return homeTrash;
    }

    const QString mount = mountPointOf(path);
    if (mountPoint)
        *mountPoint = mount;
    if (mount.isEmpty())
        return homeTrash;

    // <mount>/.Trash-$uid is preferred; <mount>/.Trash/$uid is the fallback.
    const QString perUser = mount + QStringLiteral("/.Trash-") + currentUserName();
    const QString shared = mount + QStringLiteral("/.Trash/") + currentUserName();
    for (const QString &candidate : {perUser, shared}) {
        const QFileInfo info(candidate);
        if (info.exists()) {
            if (info.isDir() && info.isWritable())
                return candidate;
            continue;
        }
        if (QDir().mkpath(candidate))
            return candidate;
    }
    return homeTrash;
}

QString TrashService::uniqueName(const QString &dir, const QString &name) const
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
    while (QFileInfo::exists(dir + QLatin1Char('/') + candidate)) {
        ++counter;
        candidate = base + QStringLiteral(".%1").arg(counter) + suffix;
    }
    return candidate;
}

QString TrashService::encodePath(const QString &path)
{
    // Encode everything except unreserved characters and '/'. The spec says the
    // value is a URI-style absolute path, so '/' stays readable.
    QByteArray out;
    const QByteArray utf8 = path.toUtf8();
    static const char hex[] = "0123456789ABCDEF";
    for (const char ch : utf8) {
        const unsigned char byte = static_cast<unsigned char>(ch);
        const bool unreserved = (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z')
            || (byte >= '0' && byte <= '9') || byte == '-' || byte == '_' || byte == '.'
            || byte == '~' || byte == '/';
        if (unreserved) {
            out.append(ch);
        } else {
            out.append('%');
            out.append(hex[(byte >> 4) & 0xF]);
            out.append(hex[byte & 0xF]);
        }
    }
    return QString::fromLatin1(out);
}

QString TrashService::decodePath(const QString &encoded)
{
    return QUrl::fromPercentEncoding(encoded.toLatin1());
}

bool TrashService::moveAcrossDevices(const QString &source, const QString &target, QString *error)
{
    const QFileInfo info(source);

    // A symlink is moved as a link: its target is never copied or followed.
    if (info.isSymLink()) {
        const QString linkTarget = info.symLinkTarget();
        if (!QFile::link(linkTarget, target)) {
            *error = tr("Bağlantı taşınamadı: %1").arg(source);
            return false;
        }
        if (!QFile::remove(source)) {
            *error = tr("Bağlantı kaldırılamadı: %1").arg(source);
            return false;
        }
        return true;
    }

    if (info.isDir()) {
        if (!QDir().mkpath(target)) {
            *error = tr("Klasör oluşturulamadı: %1").arg(target);
            return false;
        }
        const QFileInfoList children = QDir(source).entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QFileInfo &child : children) {
            if (!moveAcrossDevices(child.absoluteFilePath(), target + QLatin1Char('/') + child.fileName(), error))
                return false;
        }
        if (!QDir().rmdir(source)) {
            *error = tr("Klasör kaldırılamadı: %1").arg(source);
            return false;
        }
        return true;
    }

    if (!QFile::copy(source, target)) {
        *error = tr("Dosya kopyalanamadı: %1").arg(source);
        return false;
    }
    if (!QFile::remove(source)) {
        *error = tr("Dosya kaldırılamadı: %1").arg(source);
        return false;
    }
    return true;
}

bool TrashService::moveEntry(const QString &source, const QString &filesDir, const QString &infoDir, QString *error)
{
    if (!QDir().mkpath(filesDir) || !QDir().mkpath(infoDir)) {
        *error = tr("Çöp klasörü oluşturulamadı: %1").arg(filesDir);
        return false;
    }

    const QString name = QFileInfo(source).fileName();
    const QString unique = uniqueName(filesDir, name);
    const QString target = filesDir + QLatin1Char('/') + unique;

    if (!QFile::rename(source, target)) {
        // Different filesystem (EXDEV) or a permission quirk: copy then remove.
        if (!moveAcrossDevices(source, target, error))
            return false;
    }

    // The .trashinfo file is written after the payload so a crash leaves an
    // orphan file rather than an info entry pointing at nothing.
    const QString infoPath = infoDir + QLatin1Char('/') + unique + QStringLiteral(".trashinfo");
    QSaveFile infoFile(infoPath);
    if (!infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // Roll back so the trash never holds untracked payloads.
        QFile::rename(target, source);
        *error = tr(".trashinfo yazılamadı: %1").arg(infoPath);
        return false;
    }

    const QString original = QFileInfo(source).absoluteFilePath();
    const QByteArray payload = "[Trash Info]\nPath=" + encodePath(original).toUtf8()
        + "\nDeletionDate=" + deletionStamp(QDateTime::currentDateTime()).toUtf8() + "\n";
    infoFile.write(payload);
    if (!infoFile.commit()) {
        QFile::rename(target, source);
        *error = tr(".trashinfo kaydedilemedi: %1").arg(infoPath);
        return false;
    }
    return true;
}

bool TrashService::moveToTrash(const QStringList &paths)
{
    if (paths.isEmpty())
        return fail(tr("Çöpe taşınacak öğe yok"));

    int moved = 0;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (!info.exists() && !info.isSymLink()) {
            return fail(tr("Öğe bulunamadı: %1").arg(path));
        }
        if (QDir::cleanPath(info.absoluteFilePath()) == QDir::cleanPath(homeTrashDir())) {
            return fail(tr("Çöp klasörünün kendisi çöpe taşınamaz"));
        }

        QString mount;
        const QString trashDir = trashDirFor(path, &mount);
        QString error;
        if (!moveEntry(info.absoluteFilePath(), trashDir + QStringLiteral("/files"),
                       trashDir + QStringLiteral("/info"), &error)) {
            return fail(error);
        }
        ++moved;
    }

    refresh();
    return succeed(tr("%1 öğe çöp kutusuna taşındı").arg(moved));
}

QVariantList TrashService::list() const
{
    QList<TrashEntry> entries;
    const QString trashDir = homeTrashDir();
    const QDir infoDir(trashDir + QStringLiteral("/info"));
    const QStringList infos = infoDir.entryList({QStringLiteral("*.trashinfo")}, QDir::Files);

    for (const QString &infoName : infos) {
        const QString infoPath = infoDir.filePath(infoName);
        QFile file(infoPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        TrashEntry entry;
        entry.infoPath = infoPath;
        const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
        for (const QString &line : lines) {
            if (line.startsWith(QLatin1String("Path=")))
                entry.originalPath = decodePath(line.mid(5).trimmed());
            else if (line.startsWith(QLatin1String("DeletionDate=")))
                entry.deletedAt = QDateTime::fromString(line.mid(13).trimmed(), Qt::ISODate);
        }
        if (entry.originalPath.isEmpty())
            continue;

        const QString payloadName = infoName.left(infoName.size() - QStringLiteral(".trashinfo").size());
        entry.trashedPath = trashDir + QStringLiteral("/files/") + payloadName;
        const QFileInfo payload(entry.trashedPath);
        entry.size = payload.isDir() ? 0 : payload.size();
        entry.isDir = payload.isDir();
        entries.append(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const TrashEntry &a, const TrashEntry &b) {
        return a.deletedAt > b.deletedAt;
    });

    QVariantList result;
    result.reserve(entries.size());
    for (const TrashEntry &entry : entries) {
        result.append(QVariantMap{
            {QStringLiteral("name"), QFileInfo(entry.trashedPath).fileName()},
            {QStringLiteral("trashedPath"), entry.trashedPath},
            {QStringLiteral("originalPath"), entry.originalPath},
            {QStringLiteral("deletedAt"), entry.deletedAt.toString(QStringLiteral("dd.MM.yyyy HH:mm"))},
            {QStringLiteral("size"), entry.size},
            {QStringLiteral("isDir"), entry.isDir},
        });
    }
    return result;
}

void TrashService::refresh()
{
    const int count = list().size();
    if (count != m_count) {
        m_count = count;
        emit changed();
    }
}

bool TrashService::restore(const QString &trashedPath)
{
    const QString clean = QDir::cleanPath(trashedPath);
    const QString trashDir = homeTrashDir();
    const QString marker = trashDir + QStringLiteral("/files/");
    if (!clean.startsWith(marker))
        return fail(tr("Bu öğe çöp kutusunda değil"));

    const QString payloadName = clean.mid(marker.size());
    const QString infoPath = trashDir + QStringLiteral("/info/") + payloadName + QStringLiteral(".trashinfo");

    QFile infoFile(infoPath);
    if (!infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return fail(tr(".trashinfo bulunamadı: %1").arg(payloadName));

    QString original;
    const QStringList lines = QString::fromUtf8(infoFile.readAll()).split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (line.startsWith(QLatin1String("Path="))) {
            original = decodePath(line.mid(5).trimmed());
            break;
        }
    }
    infoFile.close();
    if (original.isEmpty())
        return fail(tr(".trashinfo içinde Path yok: %1").arg(payloadName));

    if (!QDir().mkpath(QFileInfo(original).absolutePath()))
        return fail(tr("Hedef klasör oluşturulamadı: %1").arg(QFileInfo(original).absolutePath()));

    QString target = original;
    if (QFileInfo::exists(target)) {
        // Restore next to the existing file instead of overwriting it.
        const QFileInfo info(original);
        QString base = info.completeBaseName();
        QString suffix = info.suffix().isEmpty() ? QString() : QStringLiteral(".") + info.suffix();
        int counter = 1;
        do {
            ++counter;
            target = info.absolutePath() + QLatin1Char('/') + base + QStringLiteral(" (%1)").arg(counter) + suffix;
        } while (QFileInfo::exists(target));
    }

    if (!QFile::rename(clean, target)) {
        QString error;
        if (!moveAcrossDevices(clean, target, &error))
            return fail(error);
    }
    QFile::remove(infoPath);
    refresh();
    return succeed(tr("Geri yüklendi: %1").arg(QFileInfo(target).fileName()));
}

bool TrashService::empty()
{
    const QString trashDir = homeTrashDir();
    int removed = 0;

    auto clearDir = [&removed](const QString &dir) {
        const QDir entryDir(dir);
        const QFileInfoList entries = entryDir.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QFileInfo &entry : entries) {
            const QString path = entry.absoluteFilePath();
            if (entry.isDir() && !entry.isSymLink())
                QDir(path).removeRecursively();
            else
                QFile::remove(path);
            ++removed;
        }
    };

    clearDir(trashDir + QStringLiteral("/files"));
    clearDir(trashDir + QStringLiteral("/info"));
    refresh();
    return succeed(tr("%1 öğe kalıcı olarak silindi").arg(removed));
}

bool TrashService::fail(const QString &message)
{
    m_lastError = message;
    emit lastErrorChanged();
    emit finished(QStringLiteral("trash"), false, message);
    return false;
}

bool TrashService::succeed(const QString &message)
{
    m_lastError.clear();
    emit lastErrorChanged();
    emit finished(QStringLiteral("trash"), true, message);
    return true;
}

int TrashService::count() const
{
    return m_count;
}

} // namespace TagOS::Files
