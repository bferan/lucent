#pragma once

#define FMT_CONSTEVAL
#include "fmt/core.h"

#ifndef LC_DISABLE_LOGGING
#define LC_LOG(level, ...) ::lucent::Logger::Instance().Log(level, __VA_ARGS__)
#else
#define LC_LOG(level, ...) (void)0
#endif

#define LC_DEBUG(...) LC_LOG(LogLevel::kDebug, __VA_ARGS__)
#define LC_INFO(...)  LC_LOG(LogLevel::kInfo, __VA_ARGS__)
#define LC_WARN(...)  LC_LOG(LogLevel::kWarn, __VA_ARGS__)
#define LC_ERROR(...) LC_LOG(LogLevel::kError, __VA_ARGS__)

namespace lucent
{

enum class LogLevel
{
    kDebug,
    kInfo,
    kWarn,
    kError
};

// Interface to subscribe to log events
class LogListener
{
public:
    virtual void OnLog(LogLevel level, const std::string& msg) = 0;
};

// Singleton log dispatcher
class Logger
{
public:
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

    static Logger& Instance()
    {
        static Logger s_Logger;
        return s_Logger;
    }

    void Log(LogLevel level, const char* msg)
    {
        LogImpl(level, msg);
    }

    template<typename... Args>
    void Log(LogLevel level, const char* msg, Args&& ... args)
    {
        auto text = fmt::format(msg, std::forward<Args>(args)...);
        LogImpl(level, text);
    }

    void Register(LogListener* listener);

    void Unregister(LogListener* listener);

private:
    void LogImpl(LogLevel level, const std::string& msg);

    Logger() = default;

private:
    std::vector<LogListener*> m_Listeners;

};

// A log listener that forwards to stdout
class LogStdOut : public LogListener
{
public:
    LogStdOut();

    ~LogStdOut();

    void OnLog(LogLevel level, const std::string& msg) override;
};

}