#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include <gsl-lite/gsl-lite.hpp>

#include "mutils/geometry/geometry.h"

namespace MUtils
{

struct IDeviceBuffer;
struct Shape
{
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 size;
    glm::vec3 offset;
    Bounds bounds;
};

std::shared_ptr<Shape> shape_create_sphere(const glm::vec3& size, uint32_t subdivisions = 2, bool smooth = true, const glm::vec4& color = glm::vec4(1.0f), const glm::vec3& off = glm::vec3(0.0f));
std::shared_ptr<Shape> shape_create_icosphere(const glm::vec3& size, uint32_t subdivisions = 2, bool smooth = true, const glm::vec4& color = glm::vec4(1.0f), const glm::vec3& off = glm::vec3(0.0f));
std::shared_ptr<Shape> shape_create_icosahedron(const glm::vec3& size, const glm::vec4& color = glm::vec4(1.0f), const glm::vec3& off = glm::vec3(0.0f));
std::shared_ptr<Shape> shape_create_torus(const glm::vec3& size, float innerRatio, uint32_t sides = 20, uint32_t rings = 20, bool smooth = true, const glm::vec4& color = glm::vec4(1.0f), const glm::vec3& off = glm::vec3(0.0f));
std::shared_ptr<Shape> shape_create_quad(const glm::vec3& size, const glm::vec4& color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), const glm::vec3& offset = glm::vec3(0.0f, 0.0f, 0.0f));
std::shared_ptr<Shape> shape_create_box(const glm::vec3& size, const glm::vec4& color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), const glm::vec3& offset = glm::vec3(0.0f, 0.0f, 0.0f));
std::shared_ptr<Shape> shape_create_rounded_box(const glm::vec3& sz, int N = 4, float radius = .25f, const glm::vec4& color = glm::vec4(1.0f), const glm::vec3& off = glm::vec3(0.0f, 0.0f, 0.0f));
MeshPart shape_transfer(Shape& shape, gsl::not_null<IDeviceBuffer*> vertices, gsl::not_null<IDeviceBuffer*> indices);


} // MUtils
