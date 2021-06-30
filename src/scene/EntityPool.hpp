#pragma once

#include <vector>

#include "core/Lucent.hpp"

namespace lucent
{

struct Entity
{
    uint32_t index: 24 {};
    uint32_t version: 8 {};
};

inline bool operator==(Entity lhs, Entity rhs)
{
    return lhs.index == rhs.index && lhs.version == rhs.version;
}

class EntityPool
{
public:
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
            return m_Entities.emplace_back(Entity{ static_cast<uint32_t>(index), 0 });
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
    uint32_t m_LastFreeIndex = (uint32_t)-1;
    uint32_t m_NumFree = 0;
    std::vector<Entity> m_Entities;

};

}