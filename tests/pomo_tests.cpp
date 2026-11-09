#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
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

    void cycleFollowsFourSessions()
    {
        // completedToday -> {phase, nextPhase, cycleDone, session} while in focus, then in the break it earns.
        struct Row { int done; const char *next; int focusCycle, focusSession; const char *breakNext; int breakCycle, breakSession; };
        const Row rows[] = {{0, "short", 0, 1, "focus", 1, 1}, {1, "short", 1, 2, "focus", 2, 2},
                            {2, "short", 2, 3, "focus", 3, 3}, {3, "long", 3, 4, "focus", 4, 4}};
        for (const Row &row : rows) {
            QTemporaryDir dir;
            if (row.done > 0)
                write(dir.path() + "/today", QDate::currentDate().toString(Qt::ISODate).toUtf8() + " " + QByteArray::number(row.done) + "\n");
            Backend backend(dir.path() + "/today", "/usr/bin/true", 1, 2, 3);
            QCOMPARE(backend.nextPhase(), QString(row.next));
            QCOMPARE(backend.nextTotal(), backend.nextPhase() == "long" ? 3 : 2);
            QCOMPARE(backend.cycleDone(), row.focusCycle);
            QCOMPARE(backend.session(), row.focusSession);
            backend.toggle();
            QTRY_COMPARE(backend.phase(), QString(row.next));
            QCOMPARE(backend.nextPhase(), QString(row.breakNext));
            QCOMPARE(backend.nextTotal(), 1);
            QCOMPARE(backend.cycleDone(), row.breakCycle);
            QCOMPARE(backend.session(), row.breakSession);
        }
    }

    void breakAfterZeroSessionsShowsFirstSession()
    {
        QTemporaryDir dir;
        Backend backend(dir.path() + "/today", "/usr/bin/true", 60, 5, 15);
        backend.skip();
        QCOMPARE(backend.completedToday(), 0);
        QCOMPARE(backend.cycleDone(), 0);
        QCOMPARE(backend.session(), 1);
    }

    void skipAgreesWithNextPhase()
    {
        QTemporaryDir dir;
        // Right after a long break, skipping the next focus must earn a short break, not a second long one.
        write(dir.path() + "/today", QDate::currentDate().toString(Qt::ISODate).toUtf8() + " 4\n");
        Backend backend(dir.path() + "/today", "/usr/bin/true", 60, 5, 15);
        for (int i = 0; i < 6; ++i) {
            const QString expected = backend.nextPhase();
            backend.skip();
            QCOMPARE(backend.phase(), expected);
        }
        write(dir.path() + "/today", QDate::currentDate().toString(Qt::ISODate).toUtf8() + " 3\n");
        Backend third(dir.path() + "/today", "/usr/bin/true", 60, 5, 15);
        QCOMPARE(third.nextPhase(), QString("long"));
        third.skip();
        QCOMPARE(third.phase(), QString("long"));
    }

    void adjustChangesLengthAndTimeLeftWhilePaused()
    {
        QTemporaryDir dir;
        Backend backend(dir.path() + "/today", "/usr/bin/true", 25 * 60, 5 * 60, 15 * 60);
        backend.adjust(5);
        QCOMPARE(backend.total(), 30 * 60);
        QCOMPARE(backend.remaining(), 30 * 60);
        backend.skip();
        QCOMPARE(backend.total(), 5 * 60);
        backend.skip();
        QCOMPARE(backend.total(), 30 * 60);
    }

    void adjustIsIgnoredWhileRunning()
    {
        QTemporaryDir dir;
        Backend backend(dir.path() + "/today", "/usr/bin/true", 25 * 60, 5 * 60, 15 * 60);
        backend.toggle();
        backend.adjust(5);
        QCOMPARE(backend.total(), 25 * 60);
    }

    void adjustClamps()
    {
        QTemporaryDir dir;
        Backend backend(dir.path() + "/today", "/usr/bin/true", 5 * 60, 5 * 60, 15 * 60);
        backend.adjust(-100);
        QCOMPARE(backend.total(), 60);
        QCOMPARE(backend.remaining(), 60);
        backend.adjust(1000);
        QCOMPARE(backend.total(), 180 * 60);
        QCOMPARE(backend.remaining(), 180 * 60);
    }

    void statusFileMirrorsTheTimerAndVanishesOnExit()
    {
        QTemporaryDir dir;
        const QString status = dir.path() + "/status.json";
        auto read = [&] {
            QFile file(status);
            return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
        };
        {
            Backend backend(dir.path() + "/today", "/usr/bin/true", 60, 5, 15);
            QCOMPARE(read().value("running").toBool(true), false);
            QCOMPARE(read().value("deadlineMs").toInteger(-1), qint64(0));
            backend.toggle();
            const QJsonObject json = read();
            QCOMPARE(json.value("phase").toString(), QString("focus"));
            QCOMPARE(json.value("running").toBool(), true);
            QVERIFY(json.value("deadlineMs").toInteger() > QDateTime::currentMSecsSinceEpoch());
            QVERIFY(json.value("leftMs").toInteger() > 0);
            QCOMPARE(json.value("totalMs").toInteger(), qint64(60000));
            QCOMPARE(json.value("session").toInt(), 1);
            QCOMPARE(json.value("cycleDone").toInt(), 0);
            QCOMPARE(json.value("completedToday").toInt(), 0);
            QCOMPARE(json.value("pid").toInt(), int(QCoreApplication::applicationPid()));
        }
        QVERIFY(!QFile::exists(status));
    }

    void statusPathCanBeGiven()
    {
        QTemporaryDir dir;
        const QString status = dir.path() + "/run/pomo/status.json";
        Backend backend(dir.path() + "/today", "/usr/bin/true", 60, 5, 15, status);
        QVERIFY(QFile::exists(status));
        QVERIFY(!QFile::exists(dir.path() + "/status.json"));
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

    void themeReadsWholePalette()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml",
              "accent = \"#112233\"\nbackground = '#abcdef'\nmode = \"light\"\nname = \"x\"\n");
        Theme theme(dir.path());
        QCOMPARE(theme.colors().value("accent").toString(), QString("#112233"));
        QCOMPARE(theme.colors().value("background").toString(), QString("#abcdef"));
        QCOMPARE(theme.colors().size(), 2);
        QCOMPARE(theme.dark(), false);
    }

    void themeWithoutFileIsEmptyAndDark()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QVERIFY(theme.colors().isEmpty());
        QCOMPARE(theme.dark(), true);
    }

    void themeWithoutModeIsDark()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml", "background = \"#000000\"\n");
        Theme theme(dir.path());
        QCOMPARE(theme.dark(), true);
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
