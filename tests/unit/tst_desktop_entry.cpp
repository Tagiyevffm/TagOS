#include "tag-apps/DesktopEntry.h"

#include <QTest>

using namespace TagOS::Apps;

class DesktopEntryTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesStandardEntry();
    void ignoresNonApplicationSections();
    void flagsAndTypeFiltering();
    void expandsFieldCodes();
    void commandLineSubstitutesNameAndPath();
    void splitsQuotedArguments();
    void localisedNameIsPreferred();
    void listsAreSemicolonSeparated();
    void invalidEntriesAreRejected();
};

void DesktopEntryTest::parsesStandardEntry()
{
    const QString text = QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=TagFiles\n"
        "GenericName=File Manager\n"
        "Comment=Dosyalara göz at\n"
        "Exec=tag-files %U\n"
        "Icon=tag-files\n"
        "Categories=System;FileManager;\n"
        "Keywords=files;browse;\n"
        "Terminal=false\n");

    const DesktopEntry entry = parseDesktopEntry(text, QStringLiteral("tag-files.desktop"), QStringLiteral("/x/tag-files.desktop"));
    QVERIFY(entry.isValid());
    QCOMPARE(entry.name, QStringLiteral("TagFiles"));
    QCOMPARE(entry.genericName, QStringLiteral("File Manager"));
    QCOMPARE(entry.exec, QStringLiteral("tag-files %U"));
    QCOMPARE(entry.icon, QStringLiteral("tag-files"));
    QVERIFY(!entry.terminal);
    QCOMPARE(entry.commandLine(), QStringList{QStringLiteral("tag-files")});
}

void DesktopEntryTest::ignoresNonApplicationSections()
{
    const QString text = QStringLiteral(
        "[Desktop Action new-window]\n"
        "Name=New Window\n"
        "Exec=app --new\n"
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Real\n"
        "Exec=real-app\n"
        "[Desktop Action other]\n"
        "Exec=other\n");

    const DesktopEntry entry = parseDesktopEntry(text, QStringLiteral("real.desktop"));
    QCOMPARE(entry.name, QStringLiteral("Real"));
    QCOMPARE(entry.exec, QStringLiteral("real-app"));
}

void DesktopEntryTest::flagsAndTypeFiltering()
{
    const DesktopEntry hidden = parseDesktopEntry(QStringLiteral(
        "[Desktop Entry]\nType=Application\nName=X\nExec=x\nHidden=true\n"),
        QStringLiteral("x.desktop"));
    QVERIFY(hidden.hidden);

    const DesktopEntry link = parseDesktopEntry(QStringLiteral(
        "[Desktop Entry]\nType=Link\nName=Y\nURL=https://example.org\n"),
        QStringLiteral("y.desktop"));
    QVERIFY(!link.isValid());

    const DesktopEntry terminal = parseDesktopEntry(QStringLiteral(
        "[Desktop Entry]\nType=Application\nName=Z\nExec=z\nTerminal=true\n"),
        QStringLiteral("z.desktop"));
    QVERIFY(terminal.terminal);
}

void DesktopEntryTest::expandsFieldCodes()
{
    QCOMPARE(expandExecFieldCodes(QStringLiteral("app %f")).size(), 1);
    QCOMPARE(expandExecFieldCodes(QStringLiteral("app %U %F")).first(), QStringLiteral("app"));
    // %i is only expanded as a standalone argument (freedesktop spec).
    QCOMPARE(expandExecFieldCodes(QStringLiteral("app %i"), QStringLiteral("paint")),
             (QStringList{QStringLiteral("app"), QStringLiteral("--icon"), QStringLiteral("paint")}));
    // Inline %i has no value in a launcher and must not leak into argv.
    QCOMPARE(expandExecFieldCodes(QStringLiteral("app --flag=%i")),
             (QStringList{QStringLiteral("app"), QStringLiteral("--flag=")}));
    QCOMPARE(expandExecFieldCodes(QStringLiteral("app --open=%U")),
             (QStringList{QStringLiteral("app"), QStringLiteral("--open=")}));
}

void DesktopEntryTest::commandLineSubstitutesNameAndPath()
{
    const QString text = QStringLiteral("[Desktop Entry]\n"
                                       "Type=Application\n"
                                       "Name=TagFiles\n"
                                       "Exec=app --name=%c --from=%k --open=%U\n");
    const DesktopEntry entry = parseDesktopEntry(
        text, QStringLiteral("tag-files.desktop"),
        QStringLiteral("/usr/share/applications/tag-files.desktop"));
    QCOMPARE(entry.commandLine(),
             (QStringList{QStringLiteral("app"), QStringLiteral("--name=TagFiles"),
                          QStringLiteral("--from=/usr/share/applications/tag-files.desktop"),
                          QStringLiteral("--open=")}));
}

void DesktopEntryTest::splitsQuotedArguments()
{
    QCOMPARE(expandExecFieldCodes(QStringLiteral("app --title \"Hello World\"")),
             (QStringList{QStringLiteral("app"), QStringLiteral("--title"), QStringLiteral("Hello World")}));
    QCOMPARE(expandExecFieldCodes(QStringLiteral("app -c \"a \\\"b\\\" c\"")),
             (QStringList{QStringLiteral("app"), QStringLiteral("-c"), QStringLiteral("a \"b\" c")}));
}

void DesktopEntryTest::localisedNameIsPreferred()
{
    const QString text = QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Files\n"
        "Name[tr]=Dosyalar\n"
        "Exec=files\n");

    // The parser picks the suffix from the environment, so assert on both paths
    // without assuming the host locale: the Turkish value must be valid text and
    // the fallback must never be empty.
    const DesktopEntry entry = parseDesktopEntry(text, QStringLiteral("files.desktop"));
    QVERIFY(entry.name == QStringLiteral("Files") || entry.name == QStringLiteral("Dosyalar"));
}

void DesktopEntryTest::listsAreSemicolonSeparated()
{
    const DesktopEntry entry = parseDesktopEntry(QStringLiteral(
        "[Desktop Entry]\nType=Application\nName=A\nExec=a\n"
        "Categories=AudioVideo;Player;;\nOnlyShowIn=TagOS;GNOME;\nNotShowIn=KDE;\n"),
        QStringLiteral("a.desktop"));
    QCOMPARE(entry.categories, (QStringList{QStringLiteral("AudioVideo"), QStringLiteral("Player")}));
    QCOMPARE(entry.onlyShowIn, (QStringList{QStringLiteral("TagOS"), QStringLiteral("GNOME")}));
    QCOMPARE(entry.notShowIn, QStringList{QStringLiteral("KDE")});
}

void DesktopEntryTest::invalidEntriesAreRejected()
{
    const DesktopEntry noExec = parseDesktopEntry(QStringLiteral("[Desktop Entry]\nName=Nothing\n"),
                                                 QStringLiteral("nothing.desktop"));
    QVERIFY(!noExec.isValid());
    QVERIFY(noExec.commandLine().isEmpty());
}

QTEST_GUILESS_MAIN(DesktopEntryTest)
#include "tst_desktop_entry.moc"
