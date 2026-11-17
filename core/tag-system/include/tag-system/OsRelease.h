#pragma once

#include <QString>

namespace TagOS::System {

/// Parsed `/etc/os-release` (or `usr/lib/os-release`) data.
struct OsRelease
{
    QString id;
    QString name;
    QString prettyName;
    QString versionId;
    QString versionCodename;
    QString buildId;
    QString homeUrl;

    bool isValid() const { return !id.isEmpty() || !name.isEmpty(); }
    /// `PRETTY_NAME`, falling back to `NAME`, then to `id`.
    QString displayName() const;
};

/// Pure parser — the text is passed in, so tests can use fixtures.
OsRelease parseOsRelease(const QString &text);

/// Reads and parses an os-release file. Returns an invalid struct when missing.
OsRelease readOsRelease(const QString &path = QStringLiteral("/etc/os-release"));

} // namespace TagOS::System
