#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QString>
#include <QUrl>

namespace TagOS::System {

/// Launches TagOS applications.
///
/// The shell and the launcher never call `system()`: every request goes through
/// this one object, which keeps argv handling, logging and the dry-run switch
/// (`TAGOS_DRY_RUN=1`) in a single place. Phase 3 adds application metadata
/// (desktop files, .desktop parsing); Phase 0 only needs "run this program".
class AppLauncher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool dryRun READ isDryRun CONSTANT)
    Q_PROPERTY(QStringList launched READ launched NOTIFY launchedChanged)

public:
    explicit AppLauncher(QObject *parent = nullptr);
    ~AppLauncher() override;

    QString lastError() const;
    bool isDryRun() const;
    QStringList launched() const;

    /// Runs `command` (split with QProcess::splitCommand) detached from the shell.
    Q_INVOKABLE bool launch(const QString &command);
    /// Opens a URL/file with the desktop's default handler.
    Q_INVOKABLE bool openUrl(const QUrl &url);
    /// Opens `relativePath` inside the user's home directory ("" opens $HOME).
    Q_INVOKABLE bool openPath(const QString &relativePath);
    /// Absolute home directory, for display in panels.
    Q_INVOKABLE QString homePath() const;

signals:
    void lastErrorChanged();
    void launchedChanged();
    void launchRequested(const QString &command);

private:
    bool fail(const QString &reason);

    QString m_lastError;
    QStringList m_launched;
    bool m_dryRun = false;
};

} // namespace TagOS::System
