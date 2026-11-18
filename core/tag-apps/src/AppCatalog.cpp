#include "tag-apps/AppCatalog.h"

#include <algorithm>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QUrl>

#include "tag-system/AppLauncher.h"

namespace TagOS::Apps {
namespace {

/// Executable lookup that also accepts absolute paths (TryExec=/opt/app/bin/x).
bool executableExists(const QString &program)
{
    if (program.isEmpty())
        return true;
    if (program.startsWith(QLatin1Char('/')))
        return QFileInfo(program).isExecutable();
    return !QStandardPaths::findExecutable(program).isEmpty();
}

int scoreEntry(const DesktopEntry &entry, const QString &needle)
{
    const QString name = entry.name.toLower();
    const QString generic = entry.genericName.toLower();
    const QString comment = entry.comment.toLower();
    const QString exec = entry.exec.toLower();

    if (name == needle)
        return 1000;
    if (name.startsWith(needle))
        return 900;
    if (name.contains(needle))
        return 800;
    if (generic.contains(needle))
        return 600;
    for (const QString &keyword : entry.keywords) {
        if (keyword.toLower().contains(needle))
            return 500;
    }
    for (const QString &category : entry.categories) {
        if (category.toLower().contains(needle))
            return 400;
    }
    if (comment.contains(needle))
        return 300;
    if (exec.contains(needle))
        return 200;
    if (entry.id.toLower().contains(needle))
        return 100;
    return 0;
}

} // namespace

AppCatalog::AppCatalog(QObject *parent)
    : QObject(parent)
    , m_searchPaths(desktopSearchPaths())
{
    refresh();
}

AppCatalog::~AppCatalog() = default;

QString AppCatalog::lastError() const
{
    return m_lastError;
}

bool AppCatalog::isLoaded() const
{
    return m_loaded;
}

int AppCatalog::count() const
{
    return m_entries.size();
}

QVariantList AppCatalog::entries() const
{
    QVariantList list;
    list.reserve(m_entries.size());
    for (const DesktopEntry &entry : m_entries)
        list.append(toVariant(entry));
    return list;
}

QVariantMap AppCatalog::toVariant(const DesktopEntry &entry)
{
    QStringList keywords = entry.keywords;
    return QVariantMap{
        {QStringLiteral("id"), entry.id},
        {QStringLiteral("name"), entry.displayName()},
        {QStringLiteral("genericName"), entry.genericName},
        {QStringLiteral("comment"), entry.comment},
        {QStringLiteral("icon"), entry.icon},
        {QStringLiteral("exec"), entry.exec},
        {QStringLiteral("categories"), entry.categories},
        {QStringLiteral("keywords"), keywords},
        {QStringLiteral("terminal"), entry.terminal},
        {QStringLiteral("path"), entry.path},
    };
}

void AppCatalog::refresh()
{
    m_entries.clear();
    m_loaded = false;

    // id -> index, so a user entry in ~/.local/share overrides a system one.
    QHash<QString, int> byId;

    for (const QString &dir : m_searchPaths) {
        if (!QFileInfo(dir).isDir())
            continue;

        QDirIterator it(dir, {QStringLiteral("*.desktop")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            // Relative path with '/' -> '-' is the canonical desktop id.
            const QString relative = QDir(dir).relativeFilePath(path);
            QString id = relative;
            id.replace(QLatin1Char('/'), QLatin1Char('-'));

            const DesktopEntry entry = parseDesktopEntry(QString::fromUtf8(file.readAll()), id, path);
            if (!entry.isValid() || entry.hidden || entry.noDisplay)
                continue;
            if (!executableExists(entry.tryExec))
                continue;
            if (entry.notShowIn.contains(QStringLiteral("TagOS"), Qt::CaseInsensitive))
                continue;
            if (!entry.onlyShowIn.isEmpty()
                && !entry.onlyShowIn.contains(QStringLiteral("TagOS"), Qt::CaseInsensitive)) {
                continue;
            }

            if (const auto existing = byId.constFind(id); existing != byId.constEnd()) {
                m_entries[*existing] = entry; // higher-priority directory wins
            } else {
                byId.insert(id, m_entries.size());
                m_entries.append(entry);
            }
        }
    }

    std::sort(m_entries.begin(), m_entries.end(), [](const DesktopEntry &a, const DesktopEntry &b) {
        return a.displayName().localeAwareCompare(b.displayName()) < 0;
    });

    m_loaded = true;
    emit changed();
}

QVariantList AppCatalog::search(const QString &query, int limit) const
{
    const QString needle = query.trimmed().toLower();
    if (needle.isEmpty()) {
        QVariantList head;
        for (int i = 0; i < m_entries.size() && i < limit; ++i)
            head.append(toVariant(m_entries.at(i)));
        return head;
    }

    QList<QPair<int, int>> ranked; // score, index
    for (int i = 0; i < m_entries.size(); ++i) {
        const int score = scoreEntry(m_entries.at(i), needle);
        if (score > 0)
            ranked.append({score, i});
    }
    std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) {
        if (a.first != b.first)
            return a.first > b.first;
        return a.second < b.second;
    });

    QVariantList result;
    for (int i = 0; i < ranked.size() && i < limit; ++i)
        result.append(toVariant(m_entries.at(ranked.at(i).second)));
    return result;
}

QVariantMap AppCatalog::entry(const QString &id) const
{
    for (const DesktopEntry &entry : m_entries) {
        if (entry.id == id)
            return toVariant(entry);
    }
    return {};
}

QString AppCatalog::terminalCommand(const QString &execLine) const
{
    // Terminal applications need a terminal emulator. foot is the TagOS default;
    // fall back to common alternatives instead of failing silently.
    static const QStringList terminals = {QStringLiteral("foot"), QStringLiteral("alacritty"),
                                          QStringLiteral("kitty"), QStringLiteral("xterm")};
    for (const QString &terminal : terminals) {
        if (!QStandardPaths::findExecutable(terminal).isEmpty())
            return terminal + QStringLiteral(" -e ") + execLine;
    }
    return {};
}

bool AppCatalog::launch(const QString &id)
{
    for (const DesktopEntry &entry : m_entries) {
        if (entry.id != id)
            continue;

        QString command = entry.exec;
        if (entry.terminal) {
            command = terminalCommand(command);
            if (command.isEmpty()) {
                m_lastError = tr("Terminal emulatorü bulunamadı");
                emit lastErrorChanged();
                emit launchFailed(id, m_lastError);
                return false;
            }
        }

        emit launchRequested(id, command);
        TagOS::System::AppLauncher launcher;
        if (!launcher.launch(command)) {
            m_lastError = launcher.lastError();
            emit lastErrorChanged();
            emit launchFailed(id, m_lastError);
            return false;
        }
        return true;
    }

    m_lastError = tr("Uygulama bulunamadı: %1").arg(id);
    emit lastErrorChanged();
    emit launchFailed(id, m_lastError);
    return false;
}

bool AppCatalog::launchCommand(const QString &command)
{
    TagOS::System::AppLauncher launcher;
    if (!launcher.launch(command)) {
        m_lastError = launcher.lastError();
        emit lastErrorChanged();
        emit launchFailed(command, m_lastError);
        return false;
    }
    emit launchRequested(command, command);
    return true;
}

bool AppCatalog::launchFile(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        m_lastError = tr("Dosya bulunamadı: %1").arg(path);
        emit lastErrorChanged();
        return false;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()))) {
        m_lastError = tr("Bu dosya için uygulama bulunamadı");
        emit lastErrorChanged();
        emit launchFailed(path, m_lastError);
        return false;
    }
    return true;
}

} // namespace TagOS::Apps