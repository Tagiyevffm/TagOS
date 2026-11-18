#include "tag-apps/DesktopEntry.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QLocale>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace TagOS::Apps {
namespace {

/// Splits a raw Exec= value into argv, honouring double quotes and backslash
/// escapes the way the desktop entry spec describes.
QStringList splitCommandLine(const QString &line)
{
    QStringList result;
    QString current;
    bool inQuotes = false;
    bool hasToken = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (inQuotes) {
            if (ch == QLatin1Char('\\') && i + 1 < line.size()) {
                const QChar next = line.at(i + 1);
                if (next == QLatin1Char('"') || next == QLatin1Char('\\') || next == QLatin1Char('$')
                    || next == QLatin1Char('`')) {
                    current.append(next);
                    ++i;
                    continue;
                }
            }
            if (ch == QLatin1Char('"')) {
                inQuotes = false;
                continue;
            }
            current.append(ch);
            continue;
        }

        if (ch == QLatin1Char('"')) {
            inQuotes = true;
            hasToken = true;
            continue;
        }
        if (ch == QLatin1Char(' ')) {
            if (hasToken || !current.isEmpty()) {
                result.append(current);
                current.clear();
                hasToken = false;
            }
            continue;
        }
        current.append(ch);
    }
    if (hasToken || !current.isEmpty())
        result.append(current);
    return result;
}

QString localizedValue(const QHash<QString, QString> &keys, const QString &base, const QString &suffix)
{
    if (!suffix.isEmpty()) {
        const QString localized = keys.value(base + QLatin1Char('[') + suffix + QLatin1Char(']'));
        if (!localized.isEmpty())
            return localized;
        // tr_TR -> tr: the suffix may be the language only.
        const int underscore = suffix.indexOf(QLatin1Char('_'));
        if (underscore > 0) {
            const QString language = suffix.left(underscore);
            const QString fallback = keys.value(base + QLatin1Char('[') + language + QLatin1Char(']'));
            if (!fallback.isEmpty())
                return fallback;
        }
    }
    return keys.value(base);
}

} // namespace

QString preferredLocaleSuffix()
{
    // LC_ALL / LC_MESSAGES / LANG, e.g. "tr_TR.UTF-8" -> "tr_TR".
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString value = env.value(QStringLiteral("LC_ALL"));
    if (value.isEmpty())
        value = env.value(QStringLiteral("LC_MESSAGES"));
    if (value.isEmpty())
        value = env.value(QStringLiteral("LANG"));
    const int dot = value.indexOf(QLatin1Char('.'));
    if (dot > 0)
        value = value.left(dot);
    if (value.isEmpty() || value == QLatin1String("C") || value == QLatin1String("POSIX"))
        return QString();
    return value;
}

QString DesktopEntry::displayName() const
{
    return name.isEmpty() ? id : name;
}

QStringList DesktopEntry::commandLine() const
{
    // %c (translated name) and %k (file location) are the only field codes with
    // known values in a launcher context; substitute them before expansion.
    QString expanded = exec;
    expanded.replace(QStringLiteral("%c"), displayName());
    expanded.replace(QStringLiteral("%k"), path);
    return expandExecFieldCodes(expanded, icon);
}

QStringList expandExecFieldCodes(const QString &execLine, const QString &iconName)
{
    QStringList argv = splitCommandLine(execLine);
    QStringList result;
    result.reserve(argv.size() + 2);

    for (const QString &token : argv) {
        if (token == QLatin1String("%i")) {
            if (!iconName.isEmpty()) {
                result << QStringLiteral("--icon") << iconName;
            }
            continue;
        }
        if (token == QLatin1String("%c") || token == QLatin1String("%k") || token == QLatin1String("%f")
            || token == QLatin1String("%F") || token == QLatin1String("%u") || token == QLatin1String("%U")
            || token == QLatin1String("%d") || token == QLatin1String("%D") || token == QLatin1String("%n")
            || token == QLatin1String("%N") || token == QLatin1String("%v") || token == QLatin1String("%m")) {
            // Launch without arguments: %c/%k are informational, the rest are
            // file/URL placeholders that a launcher click does not provide.
            continue;
        }
        // Strip inline field codes, e.g. "--open=%U" -> "--open=".
        QString cleaned = token;
        for (const QString &code : {QStringLiteral("%U"), QStringLiteral("%F"), QStringLiteral("%u"),
                                    QStringLiteral("%f"), QStringLiteral("%d"), QStringLiteral("%D"),
                                    QStringLiteral("%n"), QStringLiteral("%N"), QStringLiteral("%v"),
                                    QStringLiteral("%m"), QStringLiteral("%i")}) {
            cleaned.remove(code);
        }
        if (!cleaned.isEmpty())
            result.append(cleaned);
    }
    return result;
}

DesktopEntry parseDesktopEntry(const QString &text, const QString &id, const QString &path)
{
    DesktopEntry entry;
    entry.id = id;
    entry.path = path;

    QHash<QString, QString> keys;
    bool inDesktopEntry = false;

    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            inDesktopEntry = line.mid(1, line.size() - 2) == QLatin1String("Desktop Entry");
            continue;
        }
        if (!inDesktopEntry)
            continue;
        const int equals = line.indexOf(QLatin1Char('='));
        if (equals <= 0)
            continue;
        keys.insert(line.left(equals).trimmed(), line.mid(equals + 1).trimmed());
    }

    const QString suffix = preferredLocaleSuffix();
    // Translation entries may also carry a modifier ("tr_TR@latin").
    entry.name = localizedValue(keys, QStringLiteral("Name"), suffix);
    entry.genericName = localizedValue(keys, QStringLiteral("GenericName"), suffix);
    entry.comment = localizedValue(keys, QStringLiteral("Comment"), suffix);
    entry.exec = keys.value(QStringLiteral("Exec"));
    entry.icon = keys.value(QStringLiteral("Icon"));
    entry.tryExec = keys.value(QStringLiteral("TryExec"));
    entry.workingDir = keys.value(QStringLiteral("Path"));

    auto list = [&keys](const QString &key) {
        return keys.value(key).split(QLatin1Char(';'), Qt::SkipEmptyParts);
    };
    entry.categories = list(QStringLiteral("Categories"));
    entry.keywords = list(QStringLiteral("Keywords"));
    entry.onlyShowIn = list(QStringLiteral("OnlyShowIn"));
    entry.notShowIn = list(QStringLiteral("NotShowIn"));

    auto flag = [&keys](const QString &key) {
        return keys.value(key).compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    };
    entry.terminal = flag(QStringLiteral("Terminal"));
    entry.hidden = flag(QStringLiteral("Hidden"));
    entry.noDisplay = flag(QStringLiteral("NoDisplay"));

    // Only Type=Application is launchable; missing Type is treated as invalid.
    const QString type = keys.value(QStringLiteral("Type"));
    if (!type.isEmpty() && type != QLatin1String("Application")) {
        entry.exec.clear();
    }
    return entry;
}

QStringList desktopSearchPaths()
{
    QStringList dirs;

    // Explicit override for development and tests.
    const QByteArray extra = qgetenv("TAGOS_APPS_PATH");
    for (const QString &dir : QString::fromLocal8Bit(extra).split(QDir::listSeparator(), Qt::SkipEmptyParts))
        dirs << dir + QStringLiteral("/applications");

    // Relocatable image / build tree: <prefix>/bin/tag-shell -> <prefix>/share.
    if (QCoreApplication::instance()) {
        const QDir appDir(QCoreApplication::applicationDirPath());
        dirs << QDir::cleanPath(appDir.absoluteFilePath(QStringLiteral("../share/applications")));
    }

    const QString home = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!home.isEmpty())
        dirs << home + QStringLiteral("/applications");
    else
        dirs << QDir::homePath() + QStringLiteral("/.local/share/applications");

    const QByteArray dataDirs = qgetenv("XDG_DATA_DIRS");
    const QStringList systemDirs = dataDirs.isEmpty()
        ? QStringList{QStringLiteral("/usr/local/share"), QStringLiteral("/usr/share")}
        : QString::fromLocal8Bit(dataDirs).split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString &dir : systemDirs)
        dirs << dir + QStringLiteral("/applications");
    dirs << QStringLiteral("/usr/share/applications");

    QStringList unique;
    for (const QString &dir : dirs) {
        if (!unique.contains(dir))
            unique.append(dir);
    }
    return unique;
}

} // namespace TagOS::Apps
