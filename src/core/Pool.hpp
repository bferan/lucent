#pragma once

#include "core/Core.hpp"

namespace lucent
{

/// A container for recyclable elements that can be used, active or free
template<typename T>
class Pool
{
public:
    using Factory = std::function<T()>;

    explicit Pool(Factory factory)
        : m_Factory(std::move(factory))
    {}

    void Reset()
    {
        m_Free.insert(m_Free.cend(), m_Active.begin(), m_Active.end());
        m_Active.clear();
    }

    T& Get()
    {
        if (m_Active.empty())
            Allocate();

        return m_Active.back();
    }

    T& Allocate()
    {
        if (!m_Free.empty())
        {
            m_Active.push_back(m_Free.back());
            m_Free.pop_back();
        }
        else
        {
            m_Active.push_back(m_Factory());
        }
        return m_Active.back();
    }

    template<typename Fn>
    void ForEach(Fn&& fn)
    {
        for (auto& value: m_Active) fn(value);
        for (auto& value: m_Free) fn(value);
    }

private:
    std::vector<T> m_Active;
    std::vector<T> m_Free;
    Factory m_Factory;

};

}