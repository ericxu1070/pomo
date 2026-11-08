#include "backend.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

Backend::Backend(const QString &statePath, const QString &notifier, int focus, int shortBreak, int longBreak,
                 QObject *parent)
    : QObject(parent),
      m_statePath(statePath.isEmpty() ? QDir::homePath() + "/.local/state/pomo/today" : statePath),
      m_notifier(notifier.isEmpty() ? QStandardPaths::findExecutable("notify-send") : notifier),
      m_focus(focus), m_short(shortBreak), m_long(longBreak) {
    if (m_notifier.isEmpty())
        m_error = tr("notify-send is missing, so Pomo can't notify you when a timer ends. Install libnotify.");
    m_leftMs = qint64(m_focus) * 1000;
    m_ticker.setInterval(200);
    connect(&m_ticker, &QTimer::timeout, this, &Backend::tick);
    load();
}

int Backend::remaining() const {
    const qint64 left = running() ? m_deadline - QDateTime::currentMSecsSinceEpoch() : m_leftMs;
    // Round up, so the display shows 25:00 at the start and 0:00 only at the end.
    return int(qMax<qint64>(0, (left + 999) / 1000));
}

int Backend::total() const {
    return m_phase == "focus" ? m_focus : m_phase == "short" ? m_short : m_long;
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
    emit changed();
}

void Backend::reset() {
    m_deadline = 0;
    m_ticker.stop();
    m_leftMs = qint64(total()) * 1000;
    emit changed();
}

// Skipping moves on without counting the session or sending a notification.
void Backend::skip() {
    if (m_phase != "focus")
        enter("focus");
    else
        enter(completedToday() > 0 && completedToday() % 4 == 0 ? "long" : "short");
}

void Backend::tick() {
    if (running() && m_deadline <= QDateTime::currentMSecsSinceEpoch())
        finish();
    else
        emit changed();
}

void Backend::finish() {
    QString message;
    if (m_phase == "focus") {
        m_completed = completedToday() + 1;
        m_day = QDate::currentDate();
        save();
        message = tr("Focus done. Take a break.");
        enter(m_completed % 4 == 0 ? "long" : "short");
    } else {
        message = tr("Break over. Back to focus.");
        enter("focus");
    }
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
