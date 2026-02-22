/**
 * engine/core/log.hpp — ロギングシステム
 *
 * レベル: Trace / Debug / Info / Warn / Error / Fatal
 * マルチスレッドセーフ、フォーマット付き
 */
#pragma once

#include "types.hpp"
#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <mutex>

namespace engine {

enum class LogLevel : u8 {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5,
};

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level) { min_level_ = level; }
    void set_file(FILE* file)     { file_ = file; }

    void log(LogLevel level, const char* file, int line, const char* fmt, ...);

private:
    Logger() = default;
    LogLevel min_level_ = LogLevel::Info;
    FILE*    file_      = stderr;
    std::mutex mutex_;
};

} // namespace engine

// ── マクロ ──────────────────────────────────────────────
#define ENG_TRACE(...) ::engine::Logger::instance().log(::engine::LogLevel::Trace, __FILE__, __LINE__, __VA_ARGS__)
#define ENG_DEBUG(...) ::engine::Logger::instance().log(::engine::LogLevel::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define ENG_INFO(...)  ::engine::Logger::instance().log(::engine::LogLevel::Info,  __FILE__, __LINE__, __VA_ARGS__)
#define ENG_WARN(...)  ::engine::Logger::instance().log(::engine::LogLevel::Warn,  __FILE__, __LINE__, __VA_ARGS__)
#define ENG_ERROR(...) ::engine::Logger::instance().log(::engine::LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)
#define ENG_FATAL(...) ::engine::Logger::instance().log(::engine::LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__)
