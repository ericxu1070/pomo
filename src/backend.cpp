#include "backend.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

Backend::Backend(const QString &statePath, const QString &notifier, int focus, int shortBreak, int longBreak,
                 const QString &statusPath, QObject *parent)
    : QObject(parent),
      m_statePath(statePath.isEmpty() ? QDir::homePath() + "/.local/state/pomo/today" : statePath),
      m_notifier(notifier.isEmpty() ? QStandardPaths::findExecutable("notify-send") : notifier),
      m_length{{"focus", focus}, {"short", shortBreak}, {"long", longBreak}} {
    if (!statusPath.isEmpty())
        m_statusPath = statusPath;
    else if (!statePath.isEmpty())
        m_statusPath = QFileInfo(statePath).absolutePath() + "/status.json";
    else if (const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation); !runtime.isEmpty())
        m_statusPath = runtime + "/pomo/status.json";
    if (m_notifier.isEmpty())
        m_error = tr("notify-send is missing, so Pomo can't notify you when a timer ends. Install libnotify.");
    m_leftMs = qint64(total()) * 1000;
    m_ticker.setInterval(200);
    connect(&m_ticker, &QTimer::timeout, this, &Backend::tick);
    load();
    writeStatus();
}

Backend::~Backend() {
    QFile::remove(m_statusPath);
}

int Backend::remaining() const {
    const qint64 left = running() ? m_deadline - QDateTime::currentMSecsSinceEpoch() : m_leftMs;
    // Round up, so the display shows 25:00 at the start and 0:00 only at the end.
    return int(qMax<qint64>(0, (left + 999) / 1000));
}

int Backend::total() const {
    return m_length.value(m_phase);
}

// The one rule for what follows the current phase, so the line the window
// shows is exactly what finish() and skip() do.
QString Backend::nextPhase() const {
    if (m_phase != "focus")
        return "focus";
    return (completedToday() + 1) % 4 == 0 ? "long" : "short";
}

int Backend::cycleDone() const {
    const int done = completedToday();
    if (m_phase == "focus")
        return done % 4;
    return done == 0 ? 0 : (done - 1) % 4 + 1;
}

int Backend::session() const {
    return m_phase == "focus" ? cycleDone() + 1 : qMax(1, cycleDone());
}

// A count saved yesterday doesn't belong to today.
int Backend::completedToday() const {
    return m_day == QDate::currentDate() ? m_completed : 0;
}

void Backend::toggle() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (running()) {
        m_leftMs = qMax<qint64>(0, m_deadline - now);
        m_deadline = 0;
        m_ticker.stop();
    } else {
        m_deadline = now + m_leftMs;
        m_ticker.start();
    }
    writeStatus();
    emit changed();
}

void Backend::reset() {
    m_deadline = 0;
    m_ticker.stop();
    m_leftMs = qint64(total()) * 1000;
    writeStatus();
    emit changed();
}

// Changes this run's length for the current phase, only while paused.
void Backend::adjust(int minutes) {
    if (running())
        return;
    const int length = qBound(60, total() + minutes * 60, 180 * 60);
    const qint64 left = m_leftMs + qint64(length - total()) * 1000;
    if (left <= 0)
        return;
    m_length[m_phase] = length;
    m_leftMs = left;
    writeStatus();
    emit changed();
}

// Skipping moves on without counting the session or sending a notification.
void Backend::skip() {
    enter(nextPhase());
}

void Backend::tick() {
    if (running() && m_deadline <= QDateTime::currentMSecsSinceEpoch())
        finish();
    else
        emit changed();
}

void Backend::finish() {
    QString message;
    const QString next = nextPhase();   // Before the count moves.
    if (m_phase == "focus") {
        m_completed = completedToday() + 1;
        m_day = QDate::currentDate();
        save();
        message = tr("Focus done. Take a break.");
    } else {
        message = tr("Break over. Back to focus.");
    }
    enter(next);
    if (!m_notifier.isEmpty() && !QProcess::startDetached(m_notifier, {"-a", "Pomo", "-i", "pomo", "Pomo", message})) {
        m_error = tr("Couldn't run %1 to send a notification.").arg(m_notifier);
        emit changed();
    }
}

// A new phase waits for Space, so a break never starts while you're away.
void Backend::enter(const QString &phase) {
    m_phase = phase;
    m_deadline = 0;
    m_ticker.stop();
    m_leftMs = qint64(total()) * 1000;
    writeStatus();
    emit changed();
}

void Backend::load() {
    QFile file(m_statePath);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QStringList parts = QString::fromUtf8(file.readAll()).split(' ', Qt::SkipEmptyParts);
    if (parts.size() == 2) {
        m_day = QDate::fromString(parts[0].trimmed(), Qt::ISODate);
        m_completed = parts[1].trimmed().toInt();
    }
}

// Written beside the target and renamed, so a crash never leaves half a file.
void Backend::save() {
    QDir().mkpath(QFileInfo(m_statePath).absolutePath());
    QSaveFile file(m_statePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QStringLiteral("%1 %2\n").arg(m_day.toString(Qt::ISODate)).arg(m_completed).toUtf8());
        if (file.commit())
            return;
    }
    m_error = tr("Couldn't save today's sessions to %1.").arg(m_statePath);
}

// For the bar widget. Failing silently is fine, it only mirrors what the window shows.
void Backend::writeStatus() {
    if (m_statusPath.isEmpty())
        return;
    QDir().mkpath(QFileInfo(m_statusPath).absolutePath());
    QSaveFile file(m_statusPath);
    if (!file.open(QIODevice::WriteOnly))
        return;
    const QJsonObject status{{"phase", m_phase},
                             {"running", running()},
                             {"deadlineMs", m_deadline},
                             {"leftMs", running() ? m_deadline - QDateTime::currentMSecsSinceEpoch() : m_leftMs},
                             {"totalMs", qint64(total()) * 1000},
                             {"session", session()},
                             {"cycleDone", cycleDone()},
                             {"completedToday", completedToday()},
                             // A killed app never deletes the file, so the widget checks this is alive.
                             {"pid", QCoreApplication::applicationPid()}};
    file.write(QJsonDocument(status).toJson(QJsonDocument::Compact));
    file.commit();
}
