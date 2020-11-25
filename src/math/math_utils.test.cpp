#include <catch2/catch.hpp>
#include "mutils/math/math_utils.h"

TEST_CASE("RectEmpty", "MathUtils")
{
    REQUIRE(IsRectEmpty(glm::vec4(0)));
}

TEST_CASE("RectContains", "MathUtils")
{
    REQUIRE(RectContains(glm::vec4(1, 2, 50, 60), glm::vec2(30, 30)));
    REQUIRE_FALSE(RectContains(glm::vec4(1, 2, 50, 60), glm::vec2(80, 30)));
}

TEST_CASE("Clamp", "MathUtils")
{
    REQUIRE(Clamp(9, 4, 8) == 8);
    REQUIRE(Clamp(6, 4, 8) == 6);
    REQUIRE(Clamp(2, 4, 8) == 4);
}

TEST_CASE("RectClip", "MathUtils")
{
    REQUIRE(RectClip(glm::vec4(3, 3, 5, 5), glm::vec4(4, 4, 2, 2)) == glm::vec4(4,4, 2, 2));
}

TEST_CASE("Bounds", "MathUtils")
{
    std::vector<glm::vec3> bounds;

    bounds.push_back(glm::vec3(.3, -.5, .6));
    bounds.push_back(glm::vec3(.9, -.8, .6));

    glm::vec3 min, max;
    GetBounds(&bounds[0], 2, min, max);
    REQUIRE_THAT(min.x, Catch::WithinULP(.3f, 1));
    REQUIRE_THAT(min.y, Catch::WithinULP(-.8f, 1));
    REQUIRE_THAT(min.z, Catch::WithinULP(.6f, 1));
    REQUIRE_THAT(max.x, Catch::WithinULP(.9f, 1));
    REQUIRE_THAT(max.y, Catch::WithinULP(-.5f, 1));
    REQUIRE_THAT(max.z, Catch::WithinULP(.6f, 1));
}
