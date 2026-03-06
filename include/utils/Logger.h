#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QMutex>
#include <QTextStream>

namespace MetaVisage {

enum class LogLevel { Debug, Info, Warning, Error, Critical };

class Logger {
public:
    static Logger& Instance();

    void Initialize(const QString& logDirectory);
    void Log(LogLevel level, const QString& message, const char* file = nullptr, int line = 0);
    void SetFileLogging(bool enabled) { fileLoggingEnabled_ = enabled; }
    void SetConsoleLogging(bool enabled) { consoleLoggingEnabled_ = enabled; }
    QString GetLogFilePath() const { return logFilePath_; }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void RotateIfNeeded();
    void WriteToFile(const QString& formattedMessage);
    static QString LevelToString(LogLevel level);

    QFile logFile_;
    QString logFilePath_;
    bool fileLoggingEnabled_ = true;
    bool consoleLoggingEnabled_ = true;
    bool initialized_ = false;
    QMutex mutex_;

    static constexpr qint64 MAX_LOG_SIZE = 10 * 1024 * 1024; // 10MB
};

// Convenience macros
#define MV_LOG_DEBUG(msg) MetaVisage::Logger::Instance().Log(MetaVisage::LogLevel::Debug, msg)
#define MV_LOG_INFO(msg) MetaVisage::Logger::Instance().Log(MetaVisage::LogLevel::Info, msg)
#define MV_LOG_WARNING(msg) MetaVisage::Logger::Instance().Log(MetaVisage::LogLevel::Warning, msg, __FILE__, __LINE__)
#define MV_LOG_ERROR(msg) MetaVisage::Logger::Instance().Log(MetaVisage::LogLevel::Error, msg, __FILE__, __LINE__)
#define MV_LOG_CRITICAL(msg) MetaVisage::Logger::Instance().Log(MetaVisage::LogLevel::Critical, msg, __FILE__, __LINE__)

} // namespace MetaVisage

#endif // LOGGER_H
