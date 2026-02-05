#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace LogInternal
{
    inline bool InitConsole()
    {
#ifdef _WIN32
        // for windows powershell
        SetConsoleOutputCP(65001); // UTF-8
#endif
        return true;
    }

    inline bool g_ConsoleInitialized = InitConsole();
}

enum class LogLevel
{
    Debug,
    Info,
    Warn,
    Error
};

namespace LogColor
{
    constexpr const char *Reset = "\033[0m";
    constexpr const char *Gray = "\033[90m";
    constexpr const char *Cyan = "\033[36m";
    constexpr const char *Green = "\033[32m";
    constexpr const char *Yellow = "\033[33m";
    constexpr const char *Red = "\033[31m";
    constexpr const char *BoldRed = "\033[1;31m";
}

class GLog
{
public:
    template <typename... Args>
    static void Print(LogLevel level, const char *file, int line, Args &&...args)
    {
        std::cout << GetTimeStamp() << " " << GetLevelColor(level) << GetLevelString(level)
                  << LogColor::Reset << " " << LogColor::Gray << "[" << ExtractFilename(file) << ":" << line << "]"
                  << LogColor::Reset << " ";
        // 折叠表达式
        (std::cout << ... << std::forward<Args>(args));
        std::cout << '\n';
    }

private:
    static std::string GetTimeStamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::tm tm_buf;
#ifdef _WIN32
        // 线程安全
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        std::ostringstream oss;
        oss << LogColor::Gray << "[" << std::put_time(&tm_buf, "%H:%M:%S") << "."
            << std::setfill('0') << std::setw(3) << ms.count()
            << "]" << LogColor::Reset;
        return oss.str();
    }

    static const char *GetLevelString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Debug:
            return "[DEBUG]";
        case LogLevel::Info:
            return "[INFO] ";
        case LogLevel::Warn:
            return "[WARN] ";
        case LogLevel::Error:
            return "[ERROR]";
        default:
            return "[?????]";
        }
    }

    static const char *GetLevelColor(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Debug:
            return LogColor::Cyan;
        case LogLevel::Info:
            return LogColor::Green;
        case LogLevel::Warn:
            return LogColor::Yellow;
        case LogLevel::Error:
            return LogColor::Red;
        default:
            return LogColor::Reset;
        }
    }

    static const char *ExtractFilename(const char *path)
    {
        const char *file = path;
        while (*path)
        {
            if (*path == '/' || *path == '\\')
            {
                file = path + 1;
            }
            path++;
        }
        return file;
    }
};

#ifndef NDEBUG
#define LOG_DEBUG(...) GLog::Print(LogLevel::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) GLog::Print(LogLevel::Info, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) GLog::Print(LogLevel::Warn, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) GLog::Print(LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARN(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#endif
