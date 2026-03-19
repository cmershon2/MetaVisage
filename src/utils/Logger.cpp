#include "utils/Logger.h"
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <iostream>

namespace MetaVisage {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
}

Logger::~Logger() {
    if (logFile_.isOpen()) {
        logFile_.close();
    }
}

void Logger::Initialize(const QString& logDirectory) {
    QMutexLocker lock(&mutex_);

    if (initialized_) return;

    // Create log directory if it doesn't exist
    QDir dir(logDirectory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    logFilePath_ = logDirectory + "/metavisage.log";

    // Rotate if existing log is too large
    QFileInfo info(logFilePath_);
    if (info.exists() && info.size() > MAX_LOG_SIZE) {
        QString oldPath = logFilePath_ + ".old";
        QFile::remove(oldPath);
        QFile::rename(logFilePath_, oldPath);
    }

    logFile_.setFileName(logFilePath_);
    if (logFile_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        initialized_ = true;
        QTextStream stream(&logFile_);
        stream << "\n--- MetaVisage session started at "
               << QDateTime::currentDateTime().toString(Qt::ISODate)
               << " ---\n";
        stream.flush();
    }
}

void Logger::Log(LogLevel level, const QString& message, const char* file, int line) {
    QMutexLocker lock(&mutex_);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = LevelToString(level);

    QString formatted;
    if (file && line > 0) {
        // Extract just the filename from the path
        QString filename = QFileInfo(file).fileName();
        formatted = QString("[%1] [%2] %3 [%4:%5]")
            .arg(timestamp, levelStr, message, filename)
            .arg(line);
    } else {
        formatted = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);
    }

    if (consoleLoggingEnabled_) {
        switch (level) {
            case LogLevel::Debug:   qDebug().noquote() << formatted; break;
            case LogLevel::Info:    qInfo().noquote() << formatted; break;
            case LogLevel::Warning: qWarning().noquote() << formatted; break;
            case LogLevel::Error:
            case LogLevel::Critical: qCritical().noquote() << formatted; break;
        }
    }

    if (fileLoggingEnabled_ && initialized_) {
        WriteToFile(formatted);
    }
}

void Logger::RotateIfNeeded() {
    if (!initialized_) return;

    QFileInfo info(logFilePath_);
    if (info.size() > MAX_LOG_SIZE) {
        logFile_.close();
        QString oldPath = logFilePath_ + ".old";
        QFile::remove(oldPath);
        QFile::rename(logFilePath_, oldPath);
        logFile_.setFileName(logFilePath_);
        (void)logFile_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
}

void Logger::WriteToFile(const QString& formattedMessage) {
    if (!logFile_.isOpen()) return;

    QTextStream stream(&logFile_);
    stream << formattedMessage << "\n";
    stream.flush();

    RotateIfNeeded();
}

QString Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT";
    }
    return "UNKNOWN";
}

} // namespace MetaVisage
