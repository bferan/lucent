#pragma once

#include "core/Lucent.hpp"

namespace lucent
{

// Encapsulates a fixed capacity array with count
template<typename T, size_t N>
class Array
{
public:
    // STL types
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;

public:
    // STL interface
    template<typename... V>
    decltype(auto) emplace_back(V&& ... vals)
    {
        LC_ASSERT(m_Count < N);
        return (m_Elems[m_Count++] = { std::forward<V>(vals)... });
    }

    void push_back(const T& value)
    {
        emplace_back(value);
    }

    void push_back(T&& value)
    {
        emplace_back(std::move(value));
    }

    const_reference operator[](size_type pos) const
    {
        LC_ASSERT(pos < m_Count);
        return m_Elems[pos];
    }

    reference operator[](size_type pos)
    {
        LC_ASSERT(pos < m_Count);
        return m_Elems[pos];
    }

    const_reference front() const
    {
        return m_Elems[0];
    }

    reference front()
    {
        return m_Elems[0];
    }

    const_reference back() const
    {
        return m_Elems[m_Count - 1];
    }

    reference back()
    {
        return m_Elems[m_Count - 1];
    }

    bool empty() const
    {
        return m_Count == 0;
    }

    T* data()
    {
        return m_Elems;
    }

    const T* data() const
    {
        return m_Elems;
    }

    iterator begin()
    {
        return m_Elems;
    }

    iterator end()
    {
        return m_Elems + m_Count;
    }

    const_iterator cbegin() const
    {
        return m_Elems;
    }

    const_iterator cend() const
    {
        return m_Elems + m_Count;
    }

    size_type size() const
    {
        return m_Count;
    }

    size_type max_size() const
    {
        return N;
    }

    size_t capacity() const
    {
        return max_size();
    }

    void clear()
    {
        std::destroy(begin(), end());
        m_Count = 0;
    }

private:
    size_t m_Count = 0;
    T m_Elems[N];
};

template<typename T, size_t N>
bool operator==(const Array<T, N>& lhs, const Array<T, N>& rhs)
{
    return lhs.size() == rhs.size() &&
        std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

}
