#include "tag-system/OsRelease.h"

#include <QFile>
#include <QStringList>

namespace TagOS::System {
namespace {

QString unquote(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2) {
        const QChar first = value.front();
        const QChar last = value.back();
        if ((first == QLatin1Char('"') && last == QLatin1Char('"'))
            || (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
            value = value.mid(1, value.size() - 2);
        }
    }
    // os-release allows shell-style escapes; only the common ones matter here.
    value.replace(QLatin1String("\\\""), QLatin1String("\""));
    value.replace(QLatin1String("\\$"), QLatin1String("$"));
    value.replace(QLatin1String("\\\\"), QLatin1String("\\"));
    return value;
}

void assign(OsRelease &release, const QString &key, const QString &value)
{
    if (key == QLatin1String("ID"))
        release.id = value;
    else if (key == QLatin1String("NAME"))
        release.name = value;
    else if (key == QLatin1String("PRETTY_NAME"))
        release.prettyName = value;
    else if (key == QLatin1String("VERSION_ID"))
        release.versionId = value;
    else if (key == QLatin1String("VERSION_CODENAME"))
        release.versionCodename = value;
    else if (key == QLatin1String("BUILD_ID"))
        release.buildId = value;
    else if (key == QLatin1String("HOME_URL"))
        release.homeUrl = value;
}

} // namespace

QString OsRelease::displayName() const
{
    if (!prettyName.isEmpty())
        return prettyName;
    if (!name.isEmpty())
        return name;
    return id;
}

OsRelease parseOsRelease(const QString &text)
{
    OsRelease release;
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const int equals = line.indexOf(QLatin1Char('='));
        if (equals <= 0)
            continue;
        const QString key = line.left(equals).trimmed();
        assign(release, key, unquote(line.mid(equals + 1)));
    }
    return release;
}

OsRelease readOsRelease(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return parseOsRelease(QString::fromUtf8(file.readAll()));
}

} // namespace TagOS::System
