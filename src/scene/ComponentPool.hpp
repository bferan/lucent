#pragma once

#include "scene/EntityIDPool.hpp"

namespace lucent
{

using ComponentID = uint32;

struct ComponentMeta
{
    inline static ComponentID s_NextID{};

    template<typename T>
    inline static const ComponentID s_ID = s_NextID++;
};

template<typename T>
class ComponentPool;


class ComponentPoolBase
{
public:
    template<typename T>
    ComponentPool<T>& As();

    auto Size();

    bool Contains(EntityID entity) const;

    virtual void Remove(EntityID entity) = 0;

    virtual ~ComponentPoolBase() = default;

public:
    auto begin()
    { return m_DenseArray.begin(); }
    auto end()
    { return m_DenseArray.end(); }

protected:
    std::vector<uint32> m_SparseArray{};
    std::vector<EntityID> m_DenseArray{};
};

template<typename T>
class ComponentPool : public ComponentPoolBase
{
public:
    static ComponentID ID()
    { return ComponentMeta::s_ID<T>; }

    ~ComponentPool() override = default;

public:
    template<typename C>
    void Assign(EntityID entity, C&& component);

    T& operator[](EntityID entity);

    void Remove(EntityID entity) override;

    void Clear();

private:
    std::vector<T> m_Components{};
};

/* Component pool base implementation: */
inline bool ComponentPoolBase::Contains(EntityID entity) const
{
    return entity.index < m_SparseArray.size() &&
        m_SparseArray[entity.index] < m_DenseArray.size() &&
        m_DenseArray[m_SparseArray[entity.index]] == entity;
}

inline auto ComponentPoolBase::Size()
{
    return m_DenseArray.size();
}

template<typename T>
ComponentPool<T>& ComponentPoolBase::As()
{
    return *reinterpret_cast<ComponentPool<T>*>(this);
}

/* Typed component pool implementation: */
template<typename T>
template<typename C>
void ComponentPool<T>::Assign(EntityID entity, C&& component)
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
T& ComponentPool<T>::operator[](EntityID entity)
{
    LC_ASSERT(Contains(entity));
    return m_Components[m_SparseArray[entity.index]];
}

template<typename T>
void ComponentPool<T>::Remove(EntityID entity)
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
