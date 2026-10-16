#include "utils/Logger.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>
#include <iostream>

namespace MetaVisage {

namespace {

// "yyyy-MM-dd hh:mm:ss.zzz"
std::string Timestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto millis = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
    const std::time_t tt = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                       tm.tm_hour, tm.tm_min, tm.tm_sec, millis);
}

// ISO 8601 (mirrors what Qt::ISODate produced).
std::string IsoTimestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t tt = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}",
                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                       tm.tm_hour, tm.tm_min, tm.tm_sec);
}

} // namespace

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void Logger::Initialize(const std::string& logDirectory) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) return;

    // Create log directory if it doesn't exist
    std::error_code ec;
    std::filesystem::create_directories(logDirectory, ec);

    logFilePath_ = logDirectory + "/metavisage.log";

    // Rotate if existing log is too large
    if (std::filesystem::exists(logFilePath_, ec)) {
        const auto size = std::filesystem::file_size(logFilePath_, ec);
        if (!ec && static_cast<std::int64_t>(size) > MAX_LOG_SIZE) {
            const std::string oldPath = logFilePath_ + ".old";
            std::filesystem::remove(oldPath, ec);
            std::filesystem::rename(logFilePath_, oldPath, ec);
        }
    }

    logFile_.open(logFilePath_, std::ios::out | std::ios::app);
    if (logFile_.is_open()) {
        initialized_ = true;
        logFile_ << "\n--- MetaVisage session started at "
                 << IsoTimestamp()
                 << " ---\n";
        logFile_.flush();
    }
}

void Logger::Log(LogLevel level, const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string timestamp = Timestamp();
    const char* levelStr = LevelToString(level);

    std::string formatted;
    if (file && line > 0) {
        // Extract just the filename from the path
        std::string filename = std::filesystem::path(file).filename().string();
        // Use the public subsystem name in diagnostic source metadata too.
        if (filename == "NRICPDeformer.cpp") filename = "WrapEngine.cpp";
        formatted = std::format("[{}] [{}] {} [{}:{}]",
                                timestamp, levelStr, message, filename, line);
    } else {
        formatted = std::format("[{}] [{}] {}", timestamp, levelStr, message);
    }

    if (consoleLoggingEnabled_) {
        std::ostream& stream = (level == LogLevel::Error || level == LogLevel::Critical)
                                   ? std::cerr
                                   : std::cout;
        stream << formatted << '\n';
    }

    if (fileLoggingEnabled_ && initialized_) {
        WriteToFile(formatted);
    }
}

void Logger::RotateIfNeeded() {
    if (!initialized_) return;

    std::error_code ec;
    const auto size = std::filesystem::file_size(logFilePath_, ec);
    if (!ec && static_cast<std::int64_t>(size) > MAX_LOG_SIZE) {
        logFile_.close();
        const std::string oldPath = logFilePath_ + ".old";
        std::filesystem::remove(oldPath, ec);
        std::filesystem::rename(logFilePath_, oldPath, ec);
        logFile_.open(logFilePath_, std::ios::out | std::ios::app);
    }
}

void Logger::WriteToFile(const std::string& formattedMessage) {
    if (!logFile_.is_open()) return;

    logFile_ << formattedMessage << '\n';
    logFile_.flush();

    RotateIfNeeded();
}

const char* Logger::LevelToString(LogLevel level) {
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
