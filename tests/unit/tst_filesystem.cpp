#include "tag-fs/FileSystem.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTest>

using namespace TagOS::Files;

class FileSystemTest : public QObject
{
    Q_OBJECT

private slots:
    void formatBytes_data();
    void formatBytes();
    void parentPathHandlesRoot();
    void dirModelListsAndFilters();
    void dirModelSortsFoldersFirst();
    void dirModelReportsErrors();
    void fileOpsCopyMoveRename();
    void fileOpsRefusesNameClash();
    void fileOpsDeleteAndRecreate();
    void searchFindsNestedFiles();
    void previewDescribesTextAndDir();
};

namespace {

QString writeFile(const QString &dir, const QString &name, const QByteArray &content)
{
    const QString path = dir + QLatin1Char('/') + name;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
        file.write(content);
    file.close();
    return path;
}

} // namespace

void FileSystemTest::formatBytes_data()
{
    QTest::addColumn<qint64>("bytes");
    QTest::addColumn<QString>("expected");

    QTest::newRow("zero") << qint64(0) << QStringLiteral("0 B");
    QTest::newRow("bytes") << qint64(512) << QStringLiteral("512 B");
    QTest::newRow("kib") << qint64(2048) << QStringLiteral("2.0 KiB");
    QTest::newRow("mib") << qint64(5 * 1024 * 1024) << QStringLiteral("5.0 MiB");
    QTest::newRow("gib") << qint64(3LL * 1024 * 1024 * 1024) << QStringLiteral("3.0 GiB");
    QTest::newRow("negative") << qint64(-1) << QStringLiteral("--");
}

void FileSystemTest::formatBytes()
{
    QFETCH(qint64, bytes);
    QFETCH(QString, expected);
    QCOMPARE(PathText::formatBytes(bytes), expected);
}

void FileSystemTest::parentPathHandlesRoot()
{
    QCOMPARE(PathText::parentOf(QStringLiteral("/home/user/file.txt")), QStringLiteral("/home/user"));
    QCOMPARE(PathText::parentOf(QStringLiteral("/home")), QStringLiteral("/"));
    QVERIFY(PathText::parentOf(QStringLiteral("/")).isEmpty());
    QVERIFY(PathText::isRoot(QStringLiteral("/")));
}

void FileSystemTest::dirModelListsAndFilters()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    writeFile(temp.path(), QStringLiteral("visible.txt"), "hello");
    writeFile(temp.path(), QStringLiteral(".hidden.txt"), "secret");
    QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("subdir")));

    DirModel model;
    QVERIFY(model.setPath(temp.path()));
    QCOMPARE(model.count(), 2);          // visible.txt + subdir

    QVariantMap first = model.at(0);
    QCOMPARE(first.value(QStringLiteral("isDir")).toBool(), true); // folders first
    QCOMPARE(first.value(QStringLiteral("name")).toString(), QStringLiteral("subdir"));

    model.setShowHidden(true);
    QCOMPARE(model.count(), 3);
    QVERIFY(model.indexOf(temp.path() + QStringLiteral("/.hidden.txt")) >= 0);
}

void FileSystemTest::dirModelSortsFoldersFirst()
{
    QTemporaryDir temp;
    writeFile(temp.path(), QStringLiteral("b.txt"), QByteArray(4096, 'x'));
    writeFile(temp.path(), QStringLiteral("a.txt"), "small");

    DirModel model;
    model.setPath(temp.path());
    model.setFoldersFirst(false);
    model.setSortBy(QStringLiteral("name"));
    QCOMPARE(model.at(0).value(QStringLiteral("name")).toString(), QStringLiteral("a.txt"));

    model.setSortBy(QStringLiteral("size"));
    QCOMPARE(model.at(0).value(QStringLiteral("name")).toString(), QStringLiteral("b.txt"));
}

void FileSystemTest::dirModelReportsErrors()
{
    DirModel model;
    QVERIFY(!model.setPath(QStringLiteral("/definitely/not/here")));
    QVERIFY(!model.lastError().isEmpty());
}

void FileSystemTest::fileOpsCopyMoveRename()
{
    QTemporaryDir temp;
    const QString source = writeFile(temp.path(), QStringLiteral("note.txt"), "TagOS");
    QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("dest")));
    const QString dest = temp.path() + QStringLiteral("/dest");

    FileOps ops;
    QSignalSpy spy(&ops, &FileOps::finished);

    QVERIFY(ops.copy({source}, dest));
    QCOMPARE(spy.count(), 1);
    QVERIFY(QFile::exists(dest + QStringLiteral("/note.txt")));

    QVERIFY(ops.rename(dest + QStringLiteral("/note.txt"), QStringLiteral("renamed.txt")));
    QVERIFY(QFile::exists(dest + QStringLiteral("/renamed.txt")));

    QVERIFY(QDir(dest).mkdir(QStringLiteral("sub")));
    QVERIFY(ops.move({dest + QStringLiteral("/renamed.txt")}, dest + QStringLiteral("/sub")));
    QVERIFY(QFile::exists(dest + QStringLiteral("/sub/renamed.txt")));
    QVERIFY(!QFile::exists(dest + QStringLiteral("/renamed.txt")));
}

void FileSystemTest::fileOpsRefusesNameClash()
{
    QTemporaryDir temp;
    const QString a = writeFile(temp.path(), QStringLiteral("a.txt"), "a");
    writeFile(temp.path(), QStringLiteral("b.txt"), "b");

    FileOps ops;
    QVERIFY(!ops.rename(a, QStringLiteral("b.txt")));
    QVERIFY(!ops.lastError().isEmpty());
    QVERIFY(QFile::exists(a));
}

void FileSystemTest::fileOpsDeleteAndRecreate()
{
    QTemporaryDir temp;
    QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("tree")));
    QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("tree/inner")));
    writeFile(temp.path() + QStringLiteral("/tree/inner"), QStringLiteral("f.txt"), "x");

    FileOps ops;
    QVERIFY(ops.removePermanently({temp.path() + QStringLiteral("/tree")}));
    QVERIFY(!QDir(temp.path() + QStringLiteral("/tree")).exists());

    QVERIFY(ops.makeDir(temp.path(), QStringLiteral("yeni")));
    QVERIFY(QDir(temp.path() + QStringLiteral("/yeni")).exists());
    QVERIFY(!ops.makeDir(temp.path(), QStringLiteral("yeni")));
}

void FileSystemTest::searchFindsNestedFiles()
{
    QTemporaryDir temp;
    QVERIFY(QDir(temp.path()).mkpath(QStringLiteral("a/b/c")));
    writeFile(temp.path() + QStringLiteral("/a/b/c"), QStringLiteral("tagos-report.txt"), "x");
    writeFile(temp.path() + QStringLiteral("/a"), QStringLiteral("other.txt"), "x");

    FileSearch search;
    QSignalSpy spy(&search, &FileSearch::resultsChanged);
    search.start(temp.path(), QStringLiteral("report"), 10);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(search.results().size(), 1);
    QCOMPARE(search.results().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("tagos-report.txt"));
    QVERIFY(!search.searching());
}

void FileSystemTest::previewDescribesTextAndDir()
{
    QTemporaryDir temp;
    writeFile(temp.path(), QStringLiteral("readme.txt"), "merhaba TagOS");

    Preview preview;
    const QVariantMap text = preview.describe(temp.path() + QStringLiteral("/readme.txt"));
    QCOMPARE(text.value(QStringLiteral("kind")).toString(), QStringLiteral("text"));
    QVERIFY(text.value(QStringLiteral("text")).toString().contains(QStringLiteral("merhaba")));

    const QVariantMap dir = preview.describe(temp.path());
    QCOMPARE(dir.value(QStringLiteral("kind")).toString(), QStringLiteral("dir"));
    QCOMPARE(dir.value(QStringLiteral("entries")).toInt(), 1);

    const QVariantMap missing = preview.describe(temp.path() + QStringLiteral("/nope"));
    QCOMPARE(missing.value(QStringLiteral("kind")).toString(), QStringLiteral("missing"));
}

QTEST_GUILESS_MAIN(FileSystemTest)
#include "tst_filesystem.moc"
