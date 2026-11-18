#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>

#include "tag-common/Export.h"

namespace TagOS::Config {

/// Persisted TagOS desktop preferences.
///
/// Backed by one JSON document (`$XDG_CONFIG_HOME/tagos/settings.json`, or
/// `TAGOS_CONFIG_HOME`), written atomically and debounced so a slider drag does
/// not hammer the disk. Every property is validated on load: a corrupt or
/// hand-edited file falls back to the default instead of breaking the session.
class TAGOS_EXPORT SettingsStore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(bool darkMode READ darkMode NOTIFY darkModeChanged)
    Q_PROPERTY(QString accent READ accent WRITE setAccent NOTIFY accentChanged)
    Q_PROPERTY(QString wallpaper READ wallpaper WRITE setWallpaper NOTIFY wallpaperChanged)
    Q_PROPERTY(QString dockPosition READ dockPosition WRITE setDockPosition NOTIFY dockPositionChanged)
    Q_PROPERTY(bool dockAutoHide READ dockAutoHide WRITE setDockAutoHide NOTIFY dockAutoHideChanged)
    Q_PROPERTY(int dockIconSize READ dockIconSize WRITE setDockIconSize NOTIFY dockIconSizeChanged)
    Q_PROPERTY(QString shellLayout READ shellLayout WRITE setShellLayout NOTIFY shellLayoutChanged)
    Q_PROPERTY(int panelHeight READ panelHeight WRITE setPanelHeight NOTIFY panelHeightChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY reducedMotionChanged)
    Q_PROPERTY(bool showSeconds READ showSeconds WRITE setShowSeconds NOTIFY showSecondsChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QStringList pinnedApps READ pinnedApps WRITE setPinnedApps NOTIFY pinnedAppsChanged)
    Q_PROPERTY(QStringList recentApps READ recentApps NOTIFY recentAppsChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit SettingsStore(QObject *parent = nullptr);
    /// Test/CLI constructor: use an explicit file path.
    explicit SettingsStore(QString configFilePath, QObject *parent = nullptr);
    ~SettingsStore() override;

    // theme
    QString themeMode() const { return m_themeMode; }
    void setThemeMode(const QString &mode);
    bool darkMode() const;
    QString accent() const { return m_accent; }
    void setAccent(const QString &accent);
    QString wallpaper() const { return m_wallpaper; }
    void setWallpaper(const QString &wallpaper);

    // shell
    QString dockPosition() const { return m_dockPosition; }
    void setDockPosition(const QString &position);
    bool dockAutoHide() const { return m_dockAutoHide; }
    void setDockAutoHide(bool autoHide);
    int dockIconSize() const { return m_dockIconSize; }
    void setDockIconSize(int size);
    QString shellLayout() const { return m_shellLayout; }
    void setShellLayout(const QString &layout);
    int panelHeight() const { return m_panelHeight; }
    void setPanelHeight(int height);
    bool reducedMotion() const { return m_reducedMotion; }
    void setReducedMotion(bool reduced);
    bool showSeconds() const { return m_showSeconds; }
    void setShowSeconds(bool show);
    int fontSize() const { return m_fontSize; }
    void setFontSize(int size);

    // apps
    QStringList pinnedApps() const { return m_pinnedApps; }
    void setPinnedApps(const QStringList &apps);
    Q_INVOKABLE void pinApp(const QString &desktopId);
    Q_INVOKABLE void unpinApp(const QString &desktopId);
    Q_INVOKABLE bool isPinned(const QString &desktopId) const;
    QStringList recentApps() const { return m_recentApps; }
    Q_INVOKABLE void rememberLaunched(const QString &desktopId);

    QString configPath() const { return m_path; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void reload();
    Q_INVOKABLE void resetToDefaults();

    /// Directory used when no explicit path is given (honours TAGOS_CONFIG_HOME).
    static QString defaultConfigDir();
    static QString resolveConfigPath();

signals:
    void themeModeChanged();
    void darkModeChanged();
    void accentChanged();
    void wallpaperChanged();
    void dockPositionChanged();
    void dockAutoHideChanged();
    void dockIconSizeChanged();
    void shellLayoutChanged();
    void panelHeightChanged();
    void reducedMotionChanged();
    void showSecondsChanged();
    void fontSizeChanged();
    void pinnedAppsChanged();
    void recentAppsChanged();
    void lastErrorChanged();
    /// Emitted once for every persisted change (used by tests and by the shell
    /// to log a single "settings saved" line instead of per-property noise).
    void changed(const QString &key);

private:
    void scheduleSave();
    void saveNow();
    void load();
    void setError(const QString &error);
    bool detectSystemDarkMode() const;

    QString m_path;
    QString m_lastError;
    QString m_themeMode = QStringLiteral("dark");
    bool m_systemDark = true;
    QString m_accent = QStringLiteral("aurora");
    QString m_wallpaper;
    QString m_dockPosition = QStringLiteral("bottom");
    bool m_dockAutoHide = false;
    int m_dockIconSize = 40;
    QString m_shellLayout = QStringLiteral("multi");
    int m_panelHeight = 40;
    bool m_reducedMotion = false;
    bool m_showSeconds = false;
    int m_fontSize = 13;
    QStringList m_pinnedApps;
    QStringList m_recentApps;
    bool m_loading = false;
};

} // namespace TagOS::Config
