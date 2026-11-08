#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

class PomoTests : public QObject {
    Q_OBJECT

private slots:
    void focusCountsAndMovesToBreak()
    {
        QTemporaryDir dir;
        Backend backend(dir.path() + "/today", "/usr/bin/true", 1, 2, 3);
        QCOMPARE(backend.phase(), QString("focus"));
        QCOMPARE(backend.remaining(), 1);
        backend.toggle();
        QVERIFY(backend.running());
        QTRY_COMPARE(backend.phase(), QString("short"));
        QVERIFY(!backend.running());
        QCOMPARE(backend.remaining(), 2);
        QCOMPARE(backend.completedToday(), 1);

        // The count survives a restart.
        Backend again(dir.path() + "/today", "/usr/bin/true");
        QCOMPARE(again.completedToday(), 1);
    }

    void yesterdaysCountIsZero()
    {
        QTemporaryDir dir;
        write(dir.path() + "/today", QDate::currentDate().addDays(-1).toString(Qt::ISODate).toUtf8() + " 7\n");
        Backend backend(dir.path() + "/today", "/usr/bin/true");
        QCOMPARE(backend.completedToday(), 0);
    }

    void fourthSessionEarnsLongBreak()
    {
        QTemporaryDir dir;
        write(dir.path() + "/today", QDate::currentDate().toString(Qt::ISODate).toUtf8() + " 3\n");
        Backend backend(dir.path() + "/today", "/usr/bin/true", 1, 2, 3);
        backend.toggle();
        QTRY_COMPARE(backend.phase(), QString("long"));
        QCOMPARE(backend.completedToday(), 4);
    }

    void pauseKeepsTimeAndSkipDoesNotCount()
    {
        QTemporaryDir dir;
        Backend backend(dir.path() + "/today", "/usr/bin/true", 60, 5, 15);
        backend.toggle();
        backend.toggle();
        QVERIFY(!backend.running());
        QCOMPARE(backend.remaining(), 60);
        backend.skip();
        QCOMPARE(backend.phase(), QString("short"));
        QCOMPARE(backend.completedToday(), 0);
        backend.skip();
        QCOMPARE(backend.phase(), QString("focus"));
    }

    void missingNotifierIsReported()
    {
        QTemporaryDir dir;
        const QByteArray path = qgetenv("PATH");
        qputenv("PATH", dir.path().toUtf8());
        Backend backend(dir.path() + "/today");
        qputenv("PATH", path);
        QVERIFY(!backend.error().isEmpty());
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QFile colors(dir.path() + "/theme/colors.toml");
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("accent = \"#112233\"\n");
        colors.close();

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        auto write = [](const QString &path, const QByteArray &data) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(write(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n"));
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(write(response, "{\"int\":0}"));

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        QVERIFY(write(response, "{\"int\":6}"));
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pt-BR\n");
        write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})");
        write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "ru\n");
        write(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})");
        write(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(PomoTests)
#include "pomo_tests.moc"
