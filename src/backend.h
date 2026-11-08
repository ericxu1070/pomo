#pragma once

#include <QDate>
#include <QObject>
#include <QTimer>

// The timer itself: which phase it is in, how long is left, and how many focus
// sessions were finished today. Time is measured against a wall-clock
// deadline, so a suspended laptop or a busy event loop never makes it drift.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString phase READ phase NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(int remaining READ remaining NOTIFY changed)
    Q_PROPERTY(int total READ total NOTIFY changed)
    Q_PROPERTY(int completedToday READ completedToday NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    // Empty statePath and notifier mean ~/.local/state/pomo/today and
    // notify-send from PATH. Durations are in seconds.
    explicit Backend(const QString &statePath = {}, const QString &notifier = {}, int focus = 25 * 60,
                     int shortBreak = 5 * 60, int longBreak = 15 * 60, QObject *parent = nullptr);

    QString phase() const { return m_phase; }
    bool running() const { return m_deadline > 0; }
    int remaining() const;
    int total() const;
    int completedToday() const;
    QString error() const { return m_error; }

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void skip();

signals:
    void changed();

private:
    void tick();
    void finish();
    void enter(const QString &phase);
    void load();
    void save();

    QString m_statePath;
    QString m_notifier;
    int m_focus, m_short, m_long;
    QString m_phase = "focus";
    qint64 m_deadline = 0;   // Epoch ms while running, 0 while paused.
    qint64 m_leftMs = 0;     // What is left while paused.
    QDate m_day;
    int m_completed = 0;
    QString m_error;
    QTimer m_ticker;
};
