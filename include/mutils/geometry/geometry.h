#pragma once

#include <glm/glm.hpp>

namespace MUtils
{

struct MeshVertex
{
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;
    glm::vec4 color;
};

struct Bounds
{
    glm::vec3 min;
    glm::vec3 max;
};

struct MeshPart
{
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
};

} // MUtils
