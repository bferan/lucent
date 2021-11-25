#include "catch2/catch_all.hpp"

#include "scene/EntityIDPool.hpp"
#include "scene/ComponentPool.hpp"

namespace lucent::tests
{

// Test components
struct C0
{
    int value;
};

struct C1
{
    int value;
};

struct C2
{
    float value;
};

TEST_CASE("Component IDs are distinct")
{
    auto id0 = ComponentMeta::s_ID<C0>;
    auto id1 = ComponentMeta::s_ID<C1>;

    REQUIRE_FALSE(id0 == id1);
}

TEST_CASE("Empty component pool")
{
    auto pool = ComponentPool<C0>();
    auto entities = EntityIDPool();

    auto entity = entities.Create();

    SECTION("Correctly initialized")
    {
        REQUIRE(pool.Size() == 0);
        REQUIRE_FALSE(pool.Contains(entity));
    }

    SECTION("Can assign entity a component")
    {
        auto component0 = C0{ 1234 };
        auto component1 = C0{ 5678 };

        pool.Assign(entity, component0);

        REQUIRE(pool.Size() == 1);
        REQUIRE(pool.Contains(entity));

        REQUIRE(pool[entity].value == component0.value);

        SECTION("Can replace value of component if already present")
        {
            pool.Assign(entity, component1);
            REQUIRE(pool[entity].value == component1.value);
        }

        SECTION("Can remove component from entity")
        {
            pool.Remove(entity);
            REQUIRE(pool.Size() == 0);
            REQUIRE_FALSE(pool.Contains(entity));
        }

        SECTION("Can clear pool")
        {
            pool.Clear();
            REQUIRE(pool.Size() == 0);
            REQUIRE_FALSE(pool.Contains(entity));
        }
    }
}

TEST_CASE("Pre-populated component pool")
{
    auto pool = ComponentPool<C0>();
    auto entities = EntityIDPool();

    constexpr auto numEntities = 10000;

    for (int i = 0; i < numEntities; ++i)
    {
        auto entity = entities.Create();
        pool.Assign(entity, C0{ i });
    }

    auto testEntity = EntityID{ .index = numEntities / 2 };

    SECTION("Correctly initialized")
    {
        REQUIRE(pool.Size() == numEntities);
        REQUIRE(pool.Contains(testEntity));
    }

    SECTION("Can replace value of component if already present")
    {
        auto component1 = C0{ 5678 };

        pool.Assign(testEntity, component1);
        REQUIRE(pool[testEntity].value == component1.value);
    }

    SECTION("Can remove component from entity")
    {
        pool.Remove(testEntity);
        REQUIRE(pool.Size() == numEntities - 1);
        REQUIRE_FALSE(pool.Contains(testEntity));
    }

    SECTION("Can clear pool")
    {
        pool.Clear();
        REQUIRE(pool.Size() == 0);
        REQUIRE_FALSE(pool.Contains(testEntity));
    }
}

}