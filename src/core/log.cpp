/**
 * src/core/log.cpp — ロギングシステム実装
 */
#include <engine/core/log.hpp>
#include <chrono>
#include <ctime>
#include <cstdarg>
#include <cstring>

namespace engine {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::log(LogLevel level, const char* file, int line, const char* fmt, ...) {
    if (level < min_level_) return;

    // タイムスタンプ
    auto now = std::chrono::system_clock::now();
    auto time_t_val = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_val);
#else
    localtime_r(&time_t_val, &tm_buf);
#endif

    char time_str[16];
    std::snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d.%03d",
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                  static_cast<int>(ms.count()));

    // レベル文字列
    const char* level_str = "???";
    const char* color = "\033[0m";
    switch (level) {
        case LogLevel::Trace: level_str = "TRACE"; color = "\033[90m"; break;
        case LogLevel::Debug: level_str = "DEBUG"; color = "\033[36m"; break;
        case LogLevel::Info:  level_str = "INFO "; color = "\033[32m"; break;
        case LogLevel::Warn:  level_str = "WARN "; color = "\033[33m"; break;
        case LogLevel::Error: level_str = "ERROR"; color = "\033[31m"; break;
        case LogLevel::Fatal: level_str = "FATAL"; color = "\033[35;1m"; break;
    }

    // ファイル名 (パスの最後の部分のみ)
    const char* fname = file;
    const char* slash = std::strrchr(file, '/');
    if (slash) fname = slash + 1;

    // メッセージフォーマット
    char msg[2048];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    std::lock_guard lock(mutex_);

    // コンソール出力 (色付き)
    std::fprintf(stderr, "%s[%s] %s %s:%d — %s\033[0m\n",
                 color, time_str, level_str, fname, line, msg);

    // ファイル出力
    if (file_ && file_ != stderr) {
        std::fprintf(file_, "[%s] %s %s:%d — %s\n",
                     time_str, level_str, fname, line, msg);
        std::fflush(file_);
    }
}

} // namespace engine
