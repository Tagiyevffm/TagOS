#pragma once

#include <QString>
#include <QStringList>

#include "tag-common/Export.h"

namespace TagOS::Apps {

/// One freedesktop.org `.desktop` entry, restricted to what a launcher needs.
struct TAGOS_EXPORT DesktopEntry
{
    QString id;            ///< canonical id, e.g. "org.tagos.TagFiles.desktop"
    QString path;          ///< file it was read from
    QString name;
    QString genericName;
    QString comment;
    QString exec;          ///< raw Exec= value
    QString icon;
    QString tryExec;
    QString workingDir;
    QStringList categories;
    QStringList keywords;
    QStringList onlyShowIn;
    QStringList notShowIn;
    bool terminal = false;
    bool hidden = false;
    bool noDisplay = false;

    bool isValid() const { return !id.isEmpty() && !exec.isEmpty(); }
    QString displayName() const;
    /// Argv for launching, with field codes expanded (%f, %U, %c, %i, ...).
    QStringList commandLine() const;
};

/// Pure parser used by the catalog and by unit tests (input is the file text).
TAGOS_EXPORT DesktopEntry parseDesktopEntry(const QString &text,
                                            const QString &id,
                                            const QString &path = QString());

/// Expands freedesktop Exec field codes. `%i` becomes `--icon <icon>`.
TAGOS_EXPORT QStringList expandExecFieldCodes(const QString &execLine,
                                             const QString &iconName = QString());

/// XDG data dirs that hold `applications/`, most specific first.
TAGOS_EXPORT QStringList desktopSearchPaths();

/// Locale suffix used for `Name[..]` keys, e.g. "tr" for tr_TR.UTF-8.
TAGOS_EXPORT QString preferredLocaleSuffix();

} // namespace TagOS::Apps
