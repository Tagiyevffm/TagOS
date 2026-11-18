#include "tag-config/SettingsStore.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

namespace TagOS::Config {
namespace {

constexpr int kSaveDebounceMs = 250;
constexpr int kRecentLimit = 12;
constexpr int kPinLimit = 24;

QStringList allowedThemeModes()
{
    return {QStringLiteral("dark"), QStringLiteral("light"), QStringLiteral("auto")};
}
QStringList allowedDockPositions()
{
    return {QStringLiteral("bottom"), QStringLiteral("left"), QStringLiteral("right")};
}
QStringList allowedAccents()
{
    return {QStringLiteral("aurora"), QStringLiteral("amber"), QStringLiteral("ice")};
}
QStringList allowedLayouts()
{
    return {QStringLiteral("multi"), QStringLiteral("single")};
}

QString normalise(const QString &value, const QStringList &allowed, const QString &fallback)
{
    return allowed.contains(value) ? value : fallback;
}

} // namespace

SettingsStore::SettingsStore(QObject *parent)
    : SettingsStore(resolveConfigPath(), parent)
{
}

SettingsStore::SettingsStore(QString configFilePath, QObject *parent)
    : QObject(parent)
    , m_path(std::move(configFilePath))
    , m_systemDark(detectSystemDarkMode())
{
    load();
}

SettingsStore::~SettingsStore() = default;

QString SettingsStore::defaultConfigDir()
{
    const QString override = QProcessEnvironment::systemEnvironment().value(QStringLiteral("TAGOS_CONFIG_HOME"));
    if (!override.isEmpty())
        return override;

    const QString xdg = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const QString base = xdg.isEmpty() ? QDir::homePath() + QStringLiteral("/.config") : xdg;
    return base + QStringLiteral("/tagos");
}

QString SettingsStore::resolveConfigPath()
{
    return defaultConfigDir() + QStringLiteral("/settings.json");
}

bool SettingsStore::detectSystemDarkMode() const
{
#if defined(Q_OS_LINUX)
    // The freedesktop colour-scheme preference is what GTK/Qt apps follow; on a
    // TagOS image it is preconfigured to "prefer-dark". When gsettings is not
    // available we default to dark, which is the TagOS identity.
    QProcess process;
    process.start(QStringLiteral("gsettings"),
                  {QStringLiteral("get"), QStringLiteral("org.gnome.desktop.interface"),
                   QStringLiteral("color-scheme")});
    if (process.waitForFinished(800)) {
        const QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        if (output.contains(QLatin1String("prefer-light")))
            return false;
    }
#endif
    return true;
}

bool SettingsStore::darkMode() const
{
    if (m_themeMode == QLatin1String("light"))
        return false;
    if (m_themeMode == QLatin1String("dark"))
        return true;
    return m_systemDark;
}

void SettingsStore::setThemeMode(const QString &mode)
{
    const QString normalised = normalise(mode, allowedThemeModes(), QStringLiteral("dark"));
    if (normalised == m_themeMode)
        return;
    const bool wasDark = darkMode();
    m_themeMode = normalised;
    emit themeModeChanged();
    if (wasDark != darkMode())
        emit darkModeChanged();
    emit changed(QStringLiteral("themeMode"));
    scheduleSave();
}

void SettingsStore::setAccent(const QString &accent)
{
    const QString normalised = normalise(accent, allowedAccents(), QStringLiteral("aurora"));
    if (normalised == m_accent)
        return;
    m_accent = normalised;
    emit accentChanged();
    emit changed(QStringLiteral("accent"));
    scheduleSave();
}

void SettingsStore::setWallpaper(const QString &wallpaper)
{
    if (wallpaper == m_wallpaper)
        return;
    m_wallpaper = wallpaper;
    emit wallpaperChanged();
    emit changed(QStringLiteral("wallpaper"));
    scheduleSave();
}

void SettingsStore::setDockPosition(const QString &position)
{
    const QString normalised = normalise(position, allowedDockPositions(), QStringLiteral("bottom"));
    if (normalised == m_dockPosition)
        return;
    m_dockPosition = normalised;
    emit dockPositionChanged();
    emit changed(QStringLiteral("dockPosition"));
    scheduleSave();
}

void SettingsStore::setDockAutoHide(bool autoHide)
{
    if (autoHide == m_dockAutoHide)
        return;
    m_dockAutoHide = autoHide;
    emit dockAutoHideChanged();
    emit changed(QStringLiteral("dockAutoHide"));
    scheduleSave();
}

void SettingsStore::setDockIconSize(int size)
{
    const int bounded = qBound(28, size, 64);
    if (bounded == m_dockIconSize)
        return;
    m_dockIconSize = bounded;
    emit dockIconSizeChanged();
    emit changed(QStringLiteral("dockIconSize"));
    scheduleSave();
}

void SettingsStore::setShellLayout(const QString &layout)
{
    const QString normalised = normalise(layout, allowedLayouts(), QStringLiteral("multi"));
    if (normalised == m_shellLayout)
        return;
    m_shellLayout = normalised;
    emit shellLayoutChanged();
    emit changed(QStringLiteral("shellLayout"));
    scheduleSave();
}

void SettingsStore::setPanelHeight(int height)
{
    const int bounded = qBound(32, height, 72);
    if (bounded == m_panelHeight)
        return;
    m_panelHeight = bounded;
    emit panelHeightChanged();
    emit changed(QStringLiteral("panelHeight"));
    scheduleSave();
}

void SettingsStore::setReducedMotion(bool reduced)
{
    if (reduced == m_reducedMotion)
        return;
    m_reducedMotion = reduced;
    emit reducedMotionChanged();
    emit changed(QStringLiteral("reducedMotion"));
    scheduleSave();
}

void SettingsStore::setShowSeconds(bool show)
{
    if (show == m_showSeconds)
        return;
    m_showSeconds = show;
    emit showSecondsChanged();
    emit changed(QStringLiteral("showSeconds"));
    scheduleSave();
}

void SettingsStore::setFontSize(int size)
{
    const int bounded = qBound(10, size, 18);
    if (bounded == m_fontSize)
        return;
    m_fontSize = bounded;
    emit fontSizeChanged();
    emit changed(QStringLiteral("fontSize"));
    scheduleSave();
}

void SettingsStore::setPinnedApps(const QStringList &apps)
{
    QStringList cleaned;
    for (const QString &id : apps) {
        if (!id.isEmpty() && !cleaned.contains(id) && cleaned.size() < kPinLimit)
            cleaned.append(id);
    }
    if (cleaned == m_pinnedApps)
        return;
    m_pinnedApps = cleaned;
    emit pinnedAppsChanged();
    emit changed(QStringLiteral("pinnedApps"));
    scheduleSave();
}

void SettingsStore::pinApp(const QString &desktopId)
{
    QStringList updated = m_pinnedApps;
    if (updated.contains(desktopId))
        return;
    updated.append(desktopId);
    setPinnedApps(updated);
}

void SettingsStore::unpinApp(const QString &desktopId)
{
    QStringList updated = m_pinnedApps;
    if (!updated.removeOne(desktopId))
        return;
    setPinnedApps(updated);
}

bool SettingsStore::isPinned(const QString &desktopId) const
{
    return m_pinnedApps.contains(desktopId);
}

void SettingsStore::rememberLaunched(const QString &desktopId)
{
    if (desktopId.isEmpty())
        return;
    QStringList updated = m_recentApps;
    updated.removeAll(desktopId);
    updated.prepend(desktopId);
    while (updated.size() > kRecentLimit)
        updated.removeLast();
    m_recentApps = updated;
    emit recentAppsChanged();
    emit changed(QStringLiteral("recentApps"));
    scheduleSave();
}

void SettingsStore::setError(const QString &error)
{
    if (error == m_lastError)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}

void SettingsStore::reload()
{
    load();
    emit themeModeChanged();
    emit darkModeChanged();
    emit accentChanged();
    emit wallpaperChanged();
    emit dockPositionChanged();
    emit dockAutoHideChanged();
    emit dockIconSizeChanged();
    emit shellLayoutChanged();
    emit panelHeightChanged();
    emit reducedMotionChanged();
    emit showSecondsChanged();
    emit fontSizeChanged();
    emit pinnedAppsChanged();
    emit recentAppsChanged();
}

void SettingsStore::resetToDefaults()
{
    m_themeMode = QStringLiteral("dark");
    m_accent = QStringLiteral("aurora");
    m_wallpaper.clear();
    m_dockPosition = QStringLiteral("bottom");
    m_dockAutoHide = false;
    m_dockIconSize = 40;
    m_shellLayout = QStringLiteral("multi");
    m_panelHeight = 40;
    m_reducedMotion = false;
    m_showSeconds = false;
    m_fontSize = 13;
    m_pinnedApps.clear();
    m_recentApps.clear();
    saveNow();
    reload();
    emit changed(QStringLiteral("*"));
}

void SettingsStore::load()
{
    m_loading = true;
    QFile file(m_path);
    if (!file.exists()) {
        m_loading = false;
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        m_loading = false;
        setError(tr("Ayarlar okunamadı: %1").arg(file.errorString()));
        return;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        m_loading = false;
        setError(tr("Ayarlar dosyası bozuk, varsayılanlar kullanılıyor: %1")
                     .arg(parseError.errorString()));
        return;
    }

    const QJsonObject object = document.object();
    m_themeMode = normalise(object.value(QStringLiteral("themeMode")).toString(),
                            allowedThemeModes(), m_themeMode);
    m_accent = normalise(object.value(QStringLiteral("accent")).toString(),
                         allowedAccents(), m_accent);
    m_wallpaper = object.value(QStringLiteral("wallpaper")).toString();
    m_dockPosition = normalise(object.value(QStringLiteral("dockPosition")).toString(),
                               allowedDockPositions(), m_dockPosition);
    m_dockAutoHide = object.value(QStringLiteral("dockAutoHide")).toBool(m_dockAutoHide);
    m_dockIconSize = qBound(28, object.value(QStringLiteral("dockIconSize")).toInt(m_dockIconSize), 64);
    m_shellLayout = normalise(object.value(QStringLiteral("shellLayout")).toString(),
                              allowedLayouts(), m_shellLayout);
    m_panelHeight = qBound(32, object.value(QStringLiteral("panelHeight")).toInt(m_panelHeight), 72);
    m_reducedMotion = object.value(QStringLiteral("reducedMotion")).toBool(m_reducedMotion);
    m_showSeconds = object.value(QStringLiteral("showSeconds")).toBool(m_showSeconds);
    m_fontSize = qBound(10, object.value(QStringLiteral("fontSize")).toInt(m_fontSize), 18);

    auto readList = [](const QJsonValue &value, int limit) {
        QStringList list;
        for (const QJsonValue &entry : value.toArray()) {
            const QString id = entry.toString();
            if (!id.isEmpty() && !list.contains(id) && list.size() < limit)
                list.append(id);
        }
        return list;
    };
    m_pinnedApps = readList(object.value(QStringLiteral("pinnedApps")), kPinLimit);
    m_recentApps = readList(object.value(QStringLiteral("recentApps")), kRecentLimit);

    setError(QString());
    m_loading = false;
}

void SettingsStore::scheduleSave()
{
    if (m_loading)
        return;
    // Coalesce bursts (theme slider, dock drag) into one atomic write.
    QTimer::singleShot(kSaveDebounceMs, this, &SettingsStore::saveNow);
}

void SettingsStore::saveNow()
{
    QJsonObject object;
    object.insert(QStringLiteral("version"), 1);
    object.insert(QStringLiteral("themeMode"), m_themeMode);
    object.insert(QStringLiteral("accent"), m_accent);
    object.insert(QStringLiteral("wallpaper"), m_wallpaper);
    object.insert(QStringLiteral("dockPosition"), m_dockPosition);
    object.insert(QStringLiteral("dockAutoHide"), m_dockAutoHide);
    object.insert(QStringLiteral("dockIconSize"), m_dockIconSize);
    object.insert(QStringLiteral("shellLayout"), m_shellLayout);
    object.insert(QStringLiteral("panelHeight"), m_panelHeight);
    object.insert(QStringLiteral("reducedMotion"), m_reducedMotion);
    object.insert(QStringLiteral("showSeconds"), m_showSeconds);
    object.insert(QStringLiteral("fontSize"), m_fontSize);
    object.insert(QStringLiteral("pinnedApps"), QJsonArray::fromStringList(m_pinnedApps));
    object.insert(QStringLiteral("recentApps"), QJsonArray::fromStringList(m_recentApps));

    const QFileInfo info(m_path);
    if (!QDir().mkpath(info.absolutePath())) {
        setError(tr("Ayar klasörü oluşturulamadı: %1").arg(info.absolutePath()));
        return;
    }

    // QSaveFile gives atomic replace: a crash mid-write never leaves a
    // half-written settings.json behind.
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Ayarlar yazılamadı: %1").arg(file.errorString()));
        return;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        setError(tr("Ayarlar kaydedilemedi: %1").arg(file.errorString()));
        return;
    }
    setError(QString());
}

} // namespace TagOS::Config
