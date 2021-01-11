#include <catch2/catch.hpp>
#include "mutils/math/math_utils.h"

using namespace MUtils;

TEST_CASE("RectEmpty", "MathUtils")
{
    REQUIRE(IsRectEmpty(NVec4f(0)));
}

TEST_CASE("RectContains", "MathUtils")
{
    REQUIRE(RectContains(NVec4f(1, 2, 50, 60), NVec2f(30, 30)));
    REQUIRE_FALSE(RectContains(NVec4f(1, 2, 50, 60), NVec2f(80, 30)));
}

TEST_CASE("Clamp", "MathUtils")
{
    REQUIRE(Clamp(9, 4, 8) == 8);
    REQUIRE(Clamp(6, 4, 8) == 6);
    REQUIRE(Clamp(2, 4, 8) == 4);
}

TEST_CASE("RectClip", "MathUtils")
{
    REQUIRE(RectClip(NVec4f(3, 3, 5, 5), NVec4f(4, 4, 2, 2)) == NVec4f(4,4, 2, 2));
}

TEST_CASE("Bounds", "MathUtils")
{
    std::vector<NVec3d> bounds;

    bounds.push_back(NVec3d(.3, -.5, .6));
    bounds.push_back(NVec3d(.9, -.8, .6));

    NVec3d min, max;
    GetBounds(&bounds[0], 2, min, max);
    REQUIRE_THAT(min.x, Catch::WithinULP(.3f, 1));
    REQUIRE_THAT(min.y, Catch::WithinULP(-.8f, 1));
    REQUIRE_THAT(min.z, Catch::WithinULP(.6f, 1));
    REQUIRE_THAT(max.x, Catch::WithinULP(.9f, 1));
    REQUIRE_THAT(max.y, Catch::WithinULP(-.5f, 1));
    REQUIRE_THAT(max.z, Catch::WithinULP(.6f, 1));
}
