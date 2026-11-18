#include "tag-fs/Trash.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

using namespace TagOS::Files;

class TrashTest : public QObject
{
    Q_OBJECT

private slots:
    void encodeDecodeRoundTrip_data();
    void encodeDecodeRoundTrip();
    void movesFileIntoTrash();
    void collisionGetsSuffix();
    void restoresToOriginalPath();
    void restoreAvoidsOverwriting();
    void emptyDeletesEverything();
    void missingPathFails();
    void refusesToTrashTheTrash();
    void keepsSymlinksAsLinks();
};

namespace {

QString makeFile(const QString &dir, const QString &name, const QByteArray &content = "tagos")
{
    const QString path = dir + QLatin1Char('/') + name;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
        file.write(content);
    file.close();
    return path;
}

} // namespace

void TrashTest::encodeDecodeRoundTrip_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("plain") << QStringLiteral("/home/user/file.txt");
    QTest::newRow("spaces") << QStringLiteral("/home/user/My Documents/rapor (1).txt");
    QTest::newRow("turkish") << QStringLiteral("/home/kullanıcı/Belgeler/şğü.txt");
    QTest::newRow("percent") << QStringLiteral("/tmp/100%25 done #1.txt");
}

void TrashTest::encodeDecodeRoundTrip()
{
    QFETCH(QString, path);
    const QString encoded = TrashService::encodePath(path);

    QVERIFY(!encoded.contains(QLatin1Char(' ')));
    QVERIFY(!encoded.contains(QLatin1Char('#')));
    QVERIFY(encoded.startsWith(QLatin1Char('/')));
    QCOMPARE(TrashService::decodePath(encoded), path);
}

void TrashTest::movesFileIntoTrash()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    const QString dataHome = workspace.filePath(QStringLiteral("data"));
    const QString source = makeFile(workspace.path(), QStringLiteral("not.txt"), "içerik");

    TrashService trash(nullptr, dataHome);
    QVERIFY(trash.moveToTrash({source}));
    QVERIFY(!QFile::exists(source));
    QCOMPARE(trash.count(), 1);

    const QString payload = dataHome + QStringLiteral("/Trash/files/not.txt");
    QVERIFY(QFile::exists(payload));

    QFile info(dataHome + QStringLiteral("/Trash/info/not.txt.trashinfo"));
    QVERIFY(info.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(info.readAll());
    QVERIFY(text.contains(QLatin1String("[Trash Info]")));
    QVERIFY(text.contains(QLatin1String("Path=") + TrashService::encodePath(source)));
    QVERIFY(text.contains(QLatin1String("DeletionDate=")));

    const QVariantList entries = trash.list();
    QCOMPARE(entries.size(), 1);
    const QVariantMap entry = entries.first().toMap();
    QCOMPARE(entry.value(QStringLiteral("originalPath")).toString(), source);
    QCOMPARE(entry.value(QStringLiteral("trashedPath")).toString(), payload);
    QVERIFY(!entry.value(QStringLiteral("deletedAt")).toString().isEmpty());
}

void TrashTest::collisionGetsSuffix()
{
    QTemporaryDir workspace;
    const QString dataHome = workspace.filePath(QStringLiteral("data"));
    const QString dirA = workspace.filePath(QStringLiteral("a"));
    const QString dirB = workspace.filePath(QStringLiteral("b"));
    QVERIFY(QDir().mkpath(dirA));
    QVERIFY(QDir().mkpath(dirB));
    const QString first = makeFile(dirA, QStringLiteral("aynı.txt"), "1");
    const QString second = makeFile(dirB, QStringLiteral("aynı.txt"), "2");

    TrashService trash(nullptr, dataHome);
    QVERIFY(trash.moveToTrash({first}));
    QVERIFY(trash.moveToTrash({second}));
    QCOMPARE(trash.count(), 2);
    QVERIFY(QFile::exists(dataHome + QStringLiteral("/Trash/files/aynı.txt")));
    QVERIFY(QFile::exists(dataHome + QStringLiteral("/Trash/files/aynı.2.txt")));
}

void TrashTest::restoresToOriginalPath()
{
    QTemporaryDir workspace;
    const QString dataHome = workspace.filePath(QStringLiteral("data"));
    const QString source = makeFile(workspace.path(), QStringLiteral("geri.txt"), "veri");

    TrashService trash(nullptr, dataHome);
    QVERIFY(trash.moveToTrash({source}));

    const QString trashed = trash.list().first().toMap().value(QStringLiteral("trashedPath")).toString();
    QVERIFY(trash.restore(trashed));
    QCOMPARE(trash.count(), 0);
    QVERIFY(QFile::exists(source));

    QFile file(source);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("veri"));
    QVERIFY(!QFile::exists(dataHome + QStringLiteral("/Trash/info/geri.txt.trashinfo")));
}

void TrashTest::restoreAvoidsOverwriting()
{
    QTemporaryDir workspace;
    const QString dataHome = workspace.filePath(QStringLiteral("data"));
    const QString source = makeFile(workspace.path(), QStringLiteral("çakışma.txt"), "eski");

    TrashService trash(nullptr, dataHome);
    QVERIFY(trash.moveToTrash({source}));

    // A new file appears at the original location before the restore.
    QFile replacement(source);
    QVERIFY(replacement.open(QIODevice::WriteOnly));
    replacement.write("yeni");
    replacement.close();

    const QString trashed = trash.list().first().toMap().value(QStringLiteral("trashedPath")).toString();
    QVERIFY(trash.restore(trashed));

    QFile original(source);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("yeni"));

    const QStringList restored = QDir(workspace.path()).entryList({QStringLiteral("çakışma (*).txt")});
    QCOMPARE(restored.size(), 1);
}

void TrashTest::emptyDeletesEverything()
{
    QTemporaryDir workspace;
    const QString dataHome = workspace.filePath(QStringLiteral("data"));
    TrashService trash(nullptr, dataHome);

    for (int i = 0; i < 3; ++i) {
        const QString file = makeFile(workspace.path(), QStringLiteral("f%1.txt").arg(i));
        QVERIFY(trash.moveToTrash({file}));
    }
    QCOMPARE(trash.count(), 3);
    QVERIFY(trash.empty());
    QCOMPARE(trash.count(), 0);
    QVERIFY(QDir(dataHome + QStringLiteral("/Trash/files")).isEmpty());
    QVERIFY(QDir(dataHome + QStringLiteral("/Trash/info")).isEmpty());
}

void TrashTest::missingPathFails()
{
    QTemporaryDir workspace;
    TrashService trash(nullptr, workspace.filePath(QStringLiteral("data")));
    QVERIFY(!trash.moveToTrash({workspace.filePath(QStringLiteral("yok.txt"))}));
    QVERIFY(trash.lastError().contains(QStringLiteral("bulunamadı"), Qt::CaseInsensitive));
    QCOMPARE(trash.count(), 0);
}

void TrashTest::refusesToTrashTheTrash()
{
    QTemporaryDir workspace;
    const QString dataHome = workspace.filePath(QStringLiteral("data"));
    TrashService trash(nullptr, dataHome);
    QVERIFY(!trash.moveToTrash({dataHome + QStringLiteral("/Trash")}));
    QVERIFY(!trash.lastError().isEmpty());
}

void TrashTest::keepsSymlinksAsLinks()
{
#ifdef Q_OS_UNIX
    QTemporaryDir workspace;
    const QString dataHome = workspace.filePath(QStringLiteral("data"));
    const QString target = makeFile(workspace.path(), QStringLiteral("hedef.txt"), "hedef");
    const QString link = workspace.filePath(QStringLiteral("bağlantı.txt"));

    if (!QFile::link(target, link))
        QSKIP("bu platformda symlink oluşturulamıyor");

    TrashService trash(nullptr, dataHome);
    QVERIFY(trash.moveToTrash({link}));

    const QString payload = dataHome + QStringLiteral("/Trash/files/bağlantı.txt");
    QVERIFY(QFileInfo(payload).isSymLink());
    QVERIFY(QFile::exists(target));   // the target itself was not moved

    QVERIFY(trash.restore(payload));
    QVERIFY(QFileInfo(link).isSymLink());
#else
    // Windows "links" are .lnk shortcuts, not symlinks: the POSIX semantics this
    // test asserts are verified on the Linux CI runner instead.
    QSKIP("symlink semantiği yalnızca POSIX'te test edilir");
#endif
}

QTEST_GUILESS_MAIN(TrashTest)
#include "tst_trash.moc"
