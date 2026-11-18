#include "tag-config/SettingsStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace TagOS::Config;

class SettingsStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreSane();
    void persistsAndReloads();
    void rejectsInvalidValues();
    void recentsAreDedupedAndBounded();
    void pinsAreUnique();
    void corruptFileFallsBackToDefaults();
    void atomicSaveCreatesParentDirectory();
    void darkModeFollowsThemeMode();
};

namespace {
QString settingsPath(const QTemporaryDir &dir)
{
    return dir.filePath(QStringLiteral("tagos/settings.json"));
}
} // namespace

void SettingsStoreTest::defaultsAreSane()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SettingsStore store(settingsPath(dir));

    QCOMPARE(store.themeMode(), QStringLiteral("dark"));
    QVERIFY(store.darkMode());
    QCOMPARE(store.dockPosition(), QStringLiteral("bottom"));
    QCOMPARE(store.dockIconSize(), 40);
    QCOMPARE(store.panelHeight(), 40);
    QVERIFY(store.pinnedApps().isEmpty());
    QVERIFY(store.configPath().endsWith(QStringLiteral("settings.json")));
}

void SettingsStoreTest::persistsAndReloads()
{
    QTemporaryDir dir;
    const QString path = settingsPath(dir);

    {
        SettingsStore store(path);
        store.setThemeMode(QStringLiteral("light"));
        store.setDockPosition(QStringLiteral("left"));
        store.setDockIconSize(52);
        store.setPanelHeight(48);
        store.setShowSeconds(true);
        store.setFontSize(15);
        store.setWallpaper(QStringLiteral("file:///tmp/aurora.png"));
        store.pinApp(QStringLiteral("tag-files.desktop"));
        store.rememberLaunched(QStringLiteral("tag-monitor.desktop"));
        QTest::qWait(400); // debounce
    }

    SettingsStore reloaded(path);
    QCOMPARE(reloaded.themeMode(), QStringLiteral("light"));
    QVERIFY(!reloaded.darkMode());
    QCOMPARE(reloaded.dockPosition(), QStringLiteral("left"));
    QCOMPARE(reloaded.dockIconSize(), 52);
    QCOMPARE(reloaded.panelHeight(), 48);
    QVERIFY(reloaded.showSeconds());
    QCOMPARE(reloaded.fontSize(), 15);
    QCOMPARE(reloaded.wallpaper(), QStringLiteral("file:///tmp/aurora.png"));
    QCOMPARE(reloaded.pinnedApps(), QStringList{QStringLiteral("tag-files.desktop")});
    QCOMPARE(reloaded.recentApps(), QStringList{QStringLiteral("tag-monitor.desktop")});
}

void SettingsStoreTest::rejectsInvalidValues()
{
    QTemporaryDir dir;
    SettingsStore store(settingsPath(dir));

    store.setThemeMode(QStringLiteral("neon"));
    QCOMPARE(store.themeMode(), QStringLiteral("dark"));

    store.setDockPosition(QStringLiteral("ceiling"));
    QCOMPARE(store.dockPosition(), QStringLiteral("bottom"));

    store.setDockIconSize(500);
    QCOMPARE(store.dockIconSize(), 64);
    store.setDockIconSize(1);
    QCOMPARE(store.dockIconSize(), 28);

    store.setShellLayout(QStringLiteral("triple"));
    QCOMPARE(store.shellLayout(), QStringLiteral("multi"));
}

void SettingsStoreTest::recentsAreDedupedAndBounded()
{
    QTemporaryDir dir;
    SettingsStore store(settingsPath(dir));

    for (int i = 0; i < 20; ++i)
        store.rememberLaunched(QStringLiteral("app%1.desktop").arg(i));

    QCOMPARE(store.recentApps().size(), 12);
    QCOMPARE(store.recentApps().first(), QStringLiteral("app19.desktop"));

    store.rememberLaunched(QStringLiteral("app5.desktop"));
    QCOMPARE(store.recentApps().first(), QStringLiteral("app5.desktop"));
    QCOMPARE(store.recentApps().count(QStringLiteral("app5.desktop")), 1);
}

void SettingsStoreTest::pinsAreUnique()
{
    QTemporaryDir dir;
    SettingsStore store(settingsPath(dir));

    store.pinApp(QStringLiteral("tag-files.desktop"));
    store.pinApp(QStringLiteral("tag-files.desktop"));
    QCOMPARE(store.pinnedApps().size(), 1);
    QVERIFY(store.isPinned(QStringLiteral("tag-files.desktop")));

    store.unpinApp(QStringLiteral("tag-files.desktop"));
    QVERIFY(store.pinnedApps().isEmpty());
    QVERIFY(!store.isPinned(QStringLiteral("tag-files.desktop")));
}

void SettingsStoreTest::corruptFileFallsBackToDefaults()
{
    QTemporaryDir dir;
    const QString path = settingsPath(dir);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ this is not json ");
    file.close();

    SettingsStore store(path);
    QCOMPARE(store.themeMode(), QStringLiteral("dark"));
    QVERIFY(!store.lastError().isEmpty());
}

void SettingsStoreTest::atomicSaveCreatesParentDirectory()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("nested/deeper/settings.json"));
    SettingsStore store(path);
    store.setAccent(QStringLiteral("ice"));
    QTest::qWait(400);

    QVERIFY(QFile::exists(path));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    QCOMPARE(object.value(QStringLiteral("accent")).toString(), QStringLiteral("ice"));
}

void SettingsStoreTest::darkModeFollowsThemeMode()
{
    QTemporaryDir dir;
    SettingsStore store(settingsPath(dir));

    store.setThemeMode(QStringLiteral("light"));
    QVERIFY(!store.darkMode());
    store.setThemeMode(QStringLiteral("dark"));
    QVERIFY(store.darkMode());
    // "auto" resolves to the detected system preference, which is dark unless
    // gsettings says otherwise.
    store.setThemeMode(QStringLiteral("auto"));
    QCOMPARE(store.darkMode(), true);
}

QTEST_GUILESS_MAIN(SettingsStoreTest)
#include "tst_settings_store.moc"
