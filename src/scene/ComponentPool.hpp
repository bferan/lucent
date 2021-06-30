#pragma once

#include <vector>

#include "core/Lucent.hpp"
#include "scene/EntityPool.hpp"

namespace lucent
{

template<typename T>
class ComponentPool
{
public:
    bool Contains(Entity entity) const;

    template<typename C>
    void Assign(Entity entity, C&& component);

    T& operator[](Entity entity);

    void Remove(Entity entity);

    void Clear();

public:
    auto begin() { return m_DenseArray.begin(); }
    auto end() { return m_DenseArray.end(); }

private:
    std::vector<uint32_t> m_SparseArray{};
    std::vector<Entity> m_DenseArray{};
    std::vector<T> m_Components{};
};

template<typename T>
bool ComponentPool<T>::Contains(Entity entity) const
{
    return entity.index < m_SparseArray.size() &&
        m_SparseArray[entity.index] < m_DenseArray.size() &&
        m_DenseArray[m_SparseArray[entity.index]] == entity;
}

template<typename T>
template<typename C>
void ComponentPool<T>::Assign(Entity entity, C&& component)
{
    if (Contains(entity))
    {
        (*this)[entity] = std::forward<C>(component);
    }
    else
    {
        auto idx = m_DenseArray.size();
        m_DenseArray.push_back(entity);
        m_Components.emplace_back(std::forward<C>(component));

        if (m_SparseArray.size() <= entity.index)
            m_SparseArray.resize(entity.index + 1);

        m_SparseArray[entity.index] = idx;
    }
}

template<typename T>
T& ComponentPool<T>::operator[](Entity entity)
{
    LC_ASSERT(Contains(entity));
    return m_Components[m_SparseArray[entity.index]];
}

template<typename T>
void ComponentPool<T>::Remove(Entity entity)
{
    LC_ASSERT(Contains(entity));

    // Swap entity with last to keep components contiguous
    auto last = m_DenseArray.back();
    auto denseIdx = m_SparseArray[entity.index];

    std::swap(m_DenseArray[denseIdx], m_DenseArray.back());
    m_DenseArray.pop_back();

    std::swap(m_Components[denseIdx], m_Components.back());
    m_Components.pop_back();

    std::swap(m_SparseArray[last.index], m_SparseArray[entity.index]);
}

template<typename T>
void ComponentPool<T>::Clear()
{
    m_SparseArray.clear();
    m_DenseArray.clear();
    m_Components.clear();
}

}
