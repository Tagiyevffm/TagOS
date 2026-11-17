#include "tag-system/OsRelease.h"

#include <QTest>

using namespace TagOS::System;

class OsReleaseTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesArchFixture();
    void stripsQuotesAndSkipsComments();
    void displayNameFallbacks();
    void missingFileIsInvalid();
};

namespace {
QString fixture(const QString &name)
{
    return QStringLiteral(TAGOS_FIXTURE_DIR) + QLatin1Char('/') + name;
}
} // namespace

void OsReleaseTest::parsesArchFixture()
{
    const OsRelease release = readOsRelease(fixture(QStringLiteral("os-release")));
    QVERIFY(release.isValid());
    QCOMPARE(release.id, QStringLiteral("arch"));
    QCOMPARE(release.prettyName, QStringLiteral("Arch Linux"));
    QCOMPARE(release.buildId, QStringLiteral("rolling"));
    QCOMPARE(release.homeUrl, QStringLiteral("https://archlinux.org/"));
}

void OsReleaseTest::stripsQuotesAndSkipsComments()
{
    const QString text = QStringLiteral("# generated\n"
                                       "\n"
                                       "ID='testos'\n"
                                       "NAME=\"Test OS\"\n"
                                       "VERSION_CODENAME=Aurora\n"
                                       "MALFORMED_LINE\n");
    const OsRelease release = parseOsRelease(text);
    QCOMPARE(release.id, QStringLiteral("testos"));
    QCOMPARE(release.name, QStringLiteral("Test OS"));
    QCOMPARE(release.versionCodename, QStringLiteral("Aurora"));
}

void OsReleaseTest::displayNameFallbacks()
{
    OsRelease release;
    QCOMPARE(release.displayName(), QString());

    release.id = QStringLiteral("tagos");
    QCOMPARE(release.displayName(), QStringLiteral("tagos"));

    release.name = QStringLiteral("TagOS Linux");
    QCOMPARE(release.displayName(), QStringLiteral("TagOS Linux"));

    release.prettyName = QStringLiteral("TagOS Linux 0.1");
    QCOMPARE(release.displayName(), QStringLiteral("TagOS Linux 0.1"));
}

void OsReleaseTest::missingFileIsInvalid()
{
    const OsRelease release = readOsRelease(fixture(QStringLiteral("nope")));
    QVERIFY(!release.isValid());
}

QTEST_GUILESS_MAIN(OsReleaseTest)
#include "tst_osrelease.moc"
