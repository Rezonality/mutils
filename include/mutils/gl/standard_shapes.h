#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include <gsl/gsl.hpp>

#include "mutils/device/geometry.h"

namespace MUtils
{

struct IDeviceBuffer;
struct Shape
{
    std::vector<MeshVertex> vertices;
    std::vector<uint16_t> indices;
    glm::vec3 size;
    glm::vec3 offset;
    Bounds bounds;
};

std::shared_ptr<Shape> shape_create_box(const glm::vec3& size = glm::vec3(1.0f, 1.0f, 1.0f), const glm::vec4& color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), const glm::vec3& offset = glm::vec3(0.0f, 0.0f, 0.0f));
MeshPart shape_transfer(Shape& shape, gsl::not_null<IDeviceBuffer*> vertices, gsl::not_null<IDeviceBuffer*> indices);


} // MUtils
