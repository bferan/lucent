#pragma once

namespace lucent
{

struct EntityID
{
    bool Empty() const
    {
        return index == 0u;
    }

    uint32 index: 24 {};
    uint32 version: 8 {};
};

inline bool operator==(EntityID lhs, EntityID rhs)
{
    return (uint32)lhs.index == rhs.index && (uint32)lhs.version == rhs.version;
}

//! Generates and recycles entity identifiers
class EntityIDPool
{
public:
    EntityIDPool()
    {
        // Add the null entity
        m_Entities.push_back(EntityID{ 0, 1 });
    }

    EntityID Create()
    {
        if (m_NumFree > 0)
        {
            auto index = m_LastFreeIndex;
            auto& entity = m_Entities[index];

            m_LastFreeIndex = entity.index;

            entity.index = index;
            entity.version++;

            m_NumFree--;

            return entity;
        }
        else
        {
            auto index = m_Entities.size();
            return m_Entities.emplace_back(EntityID{ static_cast<uint32>(index), 0 });
        }
    }

    uint32 Size() const
    {
        return m_Entities.size() - m_NumFree - 1; // Exclude the null entity
    }

    bool Valid(EntityID entity)
    {
        return entity.index < m_Entities.size() && m_Entities[entity.index] == entity;
    }

    void Destroy(EntityID entity)
    {
        LC_ASSERT(Valid(entity));

        m_Entities[entity.index].index = m_LastFreeIndex;
        m_LastFreeIndex = entity.index;

        m_NumFree++;
    }

private:
    uint32 m_LastFreeIndex = (uint32)-1;
    uint32 m_NumFree = 0;
    std::vector<EntityID> m_Entities;

};

}