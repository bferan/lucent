#include "catch2/catch_all.hpp"

#include "scene/EntityIDPool.hpp"

namespace lucent::tests
{

TEST_CASE("Empty pool")
{
    EntityIDPool pool;

    SECTION("State correctly initialized")
    {
        REQUIRE(pool.Size() == 0);
        REQUIRE_FALSE(pool.Valid(EntityID{}));
    }

    SECTION("Creation returns non-empty entity")
    {
        auto entity = pool.Create();
        REQUIRE_FALSE(entity.Empty());
        REQUIRE(pool.Size() == 1);
    }

    SECTION("EntityID is recycled")
    {
        auto one = pool.Create();
        REQUIRE(pool.Valid(one));

        auto two = pool.Create();
        REQUIRE(pool.Valid(two));
        REQUIRE(pool.Size() == 2);

        REQUIRE_FALSE(one == two);

        pool.Destroy(one);
        REQUIRE_FALSE(pool.Valid(one));
        REQUIRE(pool.Valid(two));

        auto three = pool.Create();
        REQUIRE(pool.Valid(three));
        REQUIRE_FALSE(pool.Valid(one));
    }
}

}

