#include "core/Log.hpp"

#include <iostream>

namespace lucent
{

// Logger implementation
void Logger::Register(LogListener* listener)
{
    m_Listeners.push_back(listener);
}

void Logger::Unregister(LogListener* listener)
{
    auto pos = std::find(m_Listeners.begin(), m_Listeners.end(), listener);

    if (pos != m_Listeners.end())
        m_Listeners.erase(pos);
}

void Logger::LogImpl(LogLevel level, const std::string& msg)
{
    for (auto listener : m_Listeners)
    {
        listener->OnLog(level, msg);
    }
}

// std::cout logger implementation
void LogStdOut::OnLog(LogLevel level, const std::string& msg)
{
    std::cout << msg << std::endl;
}

LogStdOut::LogStdOut()
{
    Logger::Instance().Register(this);
}

LogStdOut::~LogStdOut()
{
    Logger::Instance().Unregister(this);
}

}