#include "tag-system/AppLauncher.h"

#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace TagOS::System {

AppLauncher::AppLauncher(QObject *parent)
    : QObject(parent)
    , m_dryRun(QProcessEnvironment::systemEnvironment().value(QStringLiteral("TAGOS_DRY_RUN")) == QLatin1String("1"))
{
}

AppLauncher::~AppLauncher() = default;

QString AppLauncher::lastError() const
{
    return m_lastError;
}

bool AppLauncher::isDryRun() const
{
    return m_dryRun;
}

QStringList AppLauncher::launched() const
{
    return m_launched;
}

bool AppLauncher::fail(const QString &reason)
{
    m_lastError = reason;
    emit lastErrorChanged();
    return false;
}

bool AppLauncher::launch(const QString &command)
{
    const QString trimmed = command.trimmed();
    if (trimmed.isEmpty())
        return fail(tr("Boş komut"));

    emit launchRequested(trimmed);

    // Dry-run keeps the automated desktop tests hermetic: they assert that the
    // request reached the launcher without spawning anything.
    if (m_dryRun) {
        m_launched.append(trimmed);
        emit launchedChanged();
        return true;
    }

    const QStringList parts = QProcess::splitCommand(trimmed);
    if (parts.isEmpty())
        return fail(tr("Komut çözümlenemedi: %1").arg(trimmed));

    QString program = parts.first();
    const QStringList arguments = parts.mid(1);

    // Resolve through PATH so a missing application surfaces as a clear error
    // instead of a silent no-op.
    if (!program.startsWith(QLatin1Char('/'))) {
        const QString resolved = QStandardPaths::findExecutable(program);
        if (resolved.isEmpty())
            return fail(tr("Uygulama bulunamadı: %1").arg(program));
        program = resolved;
    }

    if (!QProcess::startDetached(program, arguments))
        return fail(tr("Başlatılamadı: %1").arg(trimmed));

    m_launched.append(trimmed);
    emit launchedChanged();
    return true;
}

bool AppLauncher::openPath(const QString &relativePath)
{
    QString absolute = QDir::homePath();
    if (!relativePath.isEmpty())
        absolute += QLatin1Char('/') + relativePath;
    if (!QDir(absolute).exists())
        return fail(tr("Klasör bulunamadı: %1").arg(absolute));
    return openUrl(QUrl::fromLocalFile(absolute));
}

QString AppLauncher::homePath() const
{
    return QDir::homePath();
}

bool AppLauncher::openUrl(const QUrl &url)
{
    if (!url.isValid())
        return fail(tr("Geçersiz adres"));
    if (m_dryRun) {
        m_launched.append(url.toString());
        emit launchedChanged();
        return true;
    }
    if (!QDesktopServices::openUrl(url))
        return fail(tr("Açılamadı: %1").arg(url.toString()));
    return true;
}

} // namespace TagOS::System
