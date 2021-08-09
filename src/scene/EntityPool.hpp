#pragma once

namespace lucent
{

struct Entity
{
    bool Empty() const
    {
        return index == 0;
    }

    uint32 index: 24 {};
    uint32 version: 8 {};
};

inline bool operator==(Entity lhs, Entity rhs)
{
    return lhs.index == rhs.index && lhs.version == rhs.version;
}

class EntityPool
{
public:
    EntityPool()
    {
        // Add the null entity
        m_Entities.push_back(Entity{ 0, 1 });
    }

    Entity Create()
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
            return m_Entities.emplace_back(Entity{ static_cast<uint32>(index), 0 });
        }
    }

    bool Valid(Entity entity)
    {
        return entity.index < m_Entities.size() && m_Entities[entity.index] == entity;
    }

    void Destroy(Entity entity)
    {
        LC_ASSERT(Valid(entity));

        m_Entities[entity.index].index = m_LastFreeIndex;
        m_LastFreeIndex = entity.index;

        m_NumFree++;
    }

private:
    uint32 m_LastFreeIndex = (uint32)-1;
    uint32 m_NumFree = 0;
    std::vector<Entity> m_Entities;

};

}