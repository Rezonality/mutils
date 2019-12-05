#include <glm/glm.hpp>
#include <cstring>

#include "mutils/device/IDeviceBuffer.h"
#include "mutils/geometry/standard_shapes.h"
#include "mutils/geometry/icosphere.h"
#include "mutils/geometry/icosahedron.h"
#include "mutils/geometry/sphere.h"
#include "mutils/geometry/rounded_box.h"

// The shapes in this file were cobbled together from various places or manually built.
// A better approach would be to make a clean mesh representation and be able to subdivide, etc.
// Or use a clean/simple library. This will do for now.
// TODO: Subdivide the quad

namespace MUtils
{

MeshPart shape_transfer(Shape& shape, gsl::not_null<IDeviceBuffer*> vertices, gsl::not_null<IDeviceBuffer*> indices)
{
    MeshPart part;
    part.indexCount = uint32_t(shape.indices.size());
    part.vertexCount = uint32_t(shape.vertices.size());

    auto pDest = vertices->Map((uint32_t)shape.vertices.size(), sizeof(MeshVertex), part.vertexOffset);
    memcpy(pDest, &shape.vertices[0], sizeof(MeshVertex) * shape.vertices.size());
    vertices->UnMap();

    pDest = indices->Map((uint32_t)shape.indices.size(), sizeof(uint32_t), part.indexOffset);
    memcpy(pDest, &shape.indices[0], sizeof(uint32_t) * shape.indices.size());
    indices->UnMap();
    return part;
}

std::shared_ptr<Shape> shape_create_icosahedron(const glm::vec3& size, const glm::vec4& color, const glm::vec3& off)
{
    Icosahedron sphere(.5f);

    auto spShape = std::make_shared<Shape>();
    spShape->size = size;
    spShape->offset = off;
    spShape->bounds.min = off - size * .5f;
    spShape->bounds.max = off + size * .5f;

    auto pVerts = sphere.getVertices();
    auto pNormals = sphere.getNormals();
    auto pTexcoords = sphere.getTexCoords();
    for (uint32_t i = 0; i < sphere.getVertexCount(); i++)
    {
        MeshVertex v;

        v.position.x = pVerts[i * 3] * size.x;
        v.position.y = pVerts[i * 3 + 1] * size.y;
        v.position.z = pVerts[i * 3 + 2] * size.z;
        v.position += off;

        v.normal.x = pNormals[i * 3];
        v.normal.y = pNormals[i * 3 + 1];
        v.normal.z = pNormals[i * 3 + 2];

        v.texcoord.x = pTexcoords[i * 3];
        v.texcoord.y = pTexcoords[i * 3 + 1];

        v.color = color;

        spShape->vertices.push_back(v);
    }

    for (uint32_t i = 0; i < sphere.getIndexCount(); i++)
    {
        spShape->indices.push_back(sphere.getIndices()[i]);
    }

    return spShape;
}

std::shared_ptr<Shape> shape_create_sphere(const glm::vec3& size, uint32_t subdivisions, bool smooth, const glm::vec4& color, const glm::vec3& off)
{
    subdivisions = std::min(subdivisions, (uint32_t)10);

    Sphere sphere(.5f, subdivisions * 8, subdivisions * 6, smooth);

    auto spShape = std::make_shared<Shape>();
    spShape->size = size;
    spShape->offset = off;
    spShape->bounds.min = off - size * .5f;
    spShape->bounds.max = off + size * .5f;

    subdivisions = std::min(subdivisions, (uint32_t)5);

    auto pVerts = sphere.getVertices();
    auto pNormals = sphere.getNormals();
    auto pTexcoords = sphere.getTexCoords();
    for (uint32_t i = 0; i < sphere.getVertexCount(); i++)
    {
        MeshVertex v;

        v.position.x = pVerts[i * 3] * size.x;
        v.position.y = pVerts[i * 3 + 1] * size.y;
        v.position.z = pVerts[i * 3 + 2] * size.z;
        v.position += off;

        v.normal.x = pNormals[i * 3];
        v.normal.y = pNormals[i * 3 + 1];
        v.normal.z = pNormals[i * 3 + 2];

        v.texcoord.x = pTexcoords[i * 2];
        v.texcoord.y = pTexcoords[i * 2 + 1];

        v.color = color;

        spShape->vertices.push_back(v);
    }

    for (uint32_t i = 0; i < sphere.getIndexCount(); i++)
    {
        spShape->indices.push_back(sphere.getIndices()[i]);
    }

    return spShape;
}

std::shared_ptr<Shape> shape_create_icosphere(const glm::vec3& size, uint32_t subdivisions, bool smooth, const glm::vec4& color, const glm::vec3& off)
{
    subdivisions = std::min(subdivisions, (uint32_t)10);

    Icosphere sphere(.5f, subdivisions, smooth);

    auto spShape = std::make_shared<Shape>();
    spShape->size = size;
    spShape->offset = off;
    spShape->bounds.min = off - size * .5f;
    spShape->bounds.max = off + size * .5f;

    subdivisions = std::min(subdivisions, (uint32_t)5);

    auto pVerts = sphere.getVertices();
    auto pNormals = sphere.getNormals();
    auto pTexcoords = sphere.getTexCoords();
    for (uint32_t i = 0; i < sphere.getVertexCount(); i++)
    {
        MeshVertex v;

        v.position.x = pVerts[i * 3] * size.x;
        v.position.y = pVerts[i * 3 + 1] * size.y;
        v.position.z = pVerts[i * 3 + 2] * size.z;
        v.position += off;

        v.normal.x = pNormals[i * 3];
        v.normal.y = pNormals[i * 3 + 1];
        v.normal.z = pNormals[i * 3 + 2];

        v.texcoord.x = pTexcoords[i * 2];
        v.texcoord.y = pTexcoords[i * 2 + 1];

        v.color = color;

        spShape->vertices.push_back(v);
    }

    for (uint32_t i = 0; i < sphere.getIndexCount(); i++)
    {
        spShape->indices.push_back(sphere.getIndices()[i]);
    }

    return spShape;
}

std::shared_ptr<Shape> shape_create_torus(const glm::vec3& size, float innerRatio, uint32_t sides, uint32_t rings, bool smooth, const glm::vec4& color, const glm::vec3& off)
{
    auto spShape = std::make_shared<Shape>();
    spShape->size = glm::vec3(size.x * 2.0f, size.y * 2.0f, size.z * 2.0f);
    spShape->offset = off;
    spShape->bounds.min = off - size * .5f;
    spShape->bounds.max = off + size * .5f;

    sides = std::min(sides, (uint32_t)50);
    rings = std::min(rings, (uint32_t)50);

    innerRatio = std::min(0.99f, innerRatio);
    innerRatio = std::max(0.01f, innerRatio);
    float outerRadius = .5f - (innerRatio * .25f);
    float innerRadius = innerRatio * .25f;

    auto AddVertex = [=](float u, float v, float innerRadius, float outerRadius)
    {
        static const float _2_PI = 6.283185307179586476925286766559f;

        static const float sTexCoord[3] = { 2.0, 0, 0 };
        static const float tTexCoord[3] = { 0, 1.0, 0 };

        MeshVertex vertex;

        float cu, su, cv, sv;

        cu = cosf(u * _2_PI);
        su = sinf(u * _2_PI);
        cv = cosf(v * _2_PI);
        sv = sinf(v * _2_PI);

        // Position
        vertex.position.x = ((outerRadius + innerRadius * cv) * cu) * size.x;
        vertex.position.y = ((outerRadius + innerRadius * cv) * su) * size.y;
        vertex.position.z = (innerRadius * sv) * size.z;

        // Normal
        vertex.normal.x = cu * cv;
        vertex.normal.y = su * cv;
        vertex.normal.z = sv;

        // Tangent
        //vertex.texcoord.x = (outerRadius + innerRadius * cv) * -su;
        //vertex.texcoord.y = (outerRadius + innerRadius * cv) * cu;

        // Binormal
        //bx = -cu * sv;
        //by = -su * cv;
        //bz = cv;

        // U, V texture mapping
        vertex.texcoord.x = (u * sTexCoord[0]) + (v * sTexCoord[1]);
        vertex.texcoord.y = (v * tTexCoord[0]) + (v * tTexCoord[1]);

        spShape->vertices.push_back(vertex);
    };

    float stepU = 1.0f / (float)sides;
    float stepV = 1.0f / (float)rings;

    float u, v;

    uint32_t index = 0;
    for (u = 0; u < 1.0f; u += stepU)
    {
        for (v = 0; v < 1.0f; v += stepV)
        {
            AddVertex(u, v, innerRadius, outerRadius);
            AddVertex(u + stepU, v, innerRadius, outerRadius);

            if (v > 0)
            {
                uint32_t indices[6] = { index - 1, index - 2, index, index - 1, index, index + 1 };

                for (int i = 0; i < 6; i++)
                {
                    spShape->indices.push_back(indices[i]);
                }

            }
            index += 2;
        }

        AddVertex(u, 1.0f, innerRadius, outerRadius);
        AddVertex(u + stepU, 1.0f, innerRadius, outerRadius);

        spShape->indices.push_back(index - 1);
        spShape->indices.push_back(index - 2);
        spShape->indices.push_back(index);
        spShape->indices.push_back(index - 1);
        spShape->indices.push_back(index);
        spShape->indices.push_back(index + 1);
        index += 2;
    }

    if (!smooth)
    {
        std::vector<MeshVertex> flatVertices;
        std::vector<uint32_t> flatIndices;
        for (uint32_t tri = 0; tri < spShape->indices.size(); tri += 3)
        {
            uint32_t indices[3] = { spShape->indices[tri], spShape->indices[tri + 1], spShape->indices[tri + 2] };
            auto v1 = spShape->vertices[indices[1]].position - spShape->vertices[indices[0]].position;
            auto v2 = spShape->vertices[indices[2]].position - spShape->vertices[indices[1]].position;

            auto norm = glm::normalize(glm::cross(v2, v1));

            for (int i = 0; i < 3; i++)
            {
                MeshVertex flat = spShape->vertices[indices[i]];
                flat.normal = norm;
                flatVertices.push_back(flat);
                flatIndices.push_back(tri + i);
            }
        }
        spShape->vertices = flatVertices;
        spShape->indices = flatIndices;
    }
    return spShape;
}

std::shared_ptr<Shape> shape_create_quad(const glm::vec3& sz, const glm::vec4& color, const glm::vec3& off)
{
    auto spShape = std::make_shared<Shape>();
    spShape->size = sz;
    spShape->offset = off;
    spShape->bounds.min = off - sz * .5f;
    spShape->bounds.max = off + sz * .5f;

    static const float quadVertices[] = {
        -1.0f, -1.0f, 0.0f,  // BL
        1.0f, -1.0f, 0.0f, // BR
        1.0f, 1.0f, 0.0f,  // TR
        -1.0f, 1.0f, 0.0f  // TL
    };

    static const float quadNormals[] = { 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f };

    static const float quadTexCoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    static const uint32_t quadIndices[] =
    {
        0, 3, 2,
        0, 2, 1
    };

    constexpr uint32_t numberVertices = sizeof(quadVertices) / (sizeof(float) * 3);
    constexpr uint32_t numberIndices = sizeof(quadIndices) / sizeof(uint32_t);

    spShape->vertices.resize(numberVertices);
    spShape->indices.resize(numberIndices);

    for (uint32_t i = 0; i < numberVertices; i++)
    {
        memcpy(&spShape->vertices[i].position, &quadVertices[i * 3], sizeof(glm::vec3));
        memcpy(&spShape->vertices[i].normal, &quadNormals[i * 3], sizeof(glm::vec3));
        memcpy(&spShape->vertices[i].texcoord, &quadTexCoords[i * 2], sizeof(glm::vec2));
        memcpy(&spShape->vertices[i].color, &color, sizeof(glm::vec4));
    }

    for (uint32_t i = 0; i < numberIndices; i++)
    {
        memcpy(&spShape->indices[i], &quadIndices[i], sizeof(uint32_t));
    }

    for (uint32_t i = 0; i < numberVertices; i++)
    {
        spShape->vertices[i].position *= (sz * .5f);
        spShape->vertices[i].position += off;
    }

    return spShape;
}

std::shared_ptr<Shape> shape_create_rounded_box(const glm::vec3& sz, int N, float radius, const glm::vec4& color, const glm::vec3& off)
{
    auto spShape = std::make_shared<Shape>();
    spShape->size = sz;
    spShape->offset = off;
    spShape->bounds.min = off - sz * .5f;
    spShape->bounds.max = off + sz * .5f;
   
    RoundCornerBox box;
    box.Create(N, sz * .5f, radius);

    spShape->vertices.resize(box.vertices.size());
    spShape->indices.resize(box.indices.size() * 3);

    for (uint32_t i = 0; i < box.vertices.size(); i++)
    {
        spShape->vertices[i].position = box.vertices[i];
        spShape->vertices[i].normal = box.normals[i];
        spShape->vertices[i].color = color;
        // TODO: Tex coords - how to calculate?
        spShape->vertices[i].texcoord = glm::vec2(0.0f);
    }

    for (uint32_t i = 0; i < box.indices.size(); i++)
    {
        spShape->indices[i * 3] = box.indices[i].x;
        spShape->indices[i * 3 + 1] = box.indices[i].y;
        spShape->indices[i * 3 + 2] = box.indices[i].z;
    }

    return spShape;
}

std::shared_ptr<Shape> shape_create_box(const glm::vec3& sz, const glm::vec4& color, const glm::vec3& off)
{
    auto spShape = std::make_shared<Shape>();
    spShape->size = sz;
    spShape->offset = off;
    spShape->bounds.min = off - sz * .5f;
    spShape->bounds.max = off + sz * .5f;

    static const float cubeVertices[] = { -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, -1.0f, +1.0f,
        -1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f,
        -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, -1.0f, +1.0f,
        -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
        -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f,
        +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f };

    static const float cubeNormals[] = { 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f,
        -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f };

    static const float cubeTangents[] = { +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
        +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f,
        0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f };

    static const float cubeTexCoords[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };

    static const uint32_t cubeIndices[] = { 0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 15, 14, 12, 14, 13, 16, 17, 18, 16, 18, 19, 20, 23, 22, 20, 22, 21 };

    constexpr uint32_t numberVertices = sizeof(cubeVertices) / (sizeof(float) * 3);
    constexpr uint32_t numberIndices = sizeof(cubeIndices) / sizeof(uint32_t);

    spShape->vertices.resize(numberVertices);
    spShape->indices.resize(numberIndices);

    for (uint32_t i = 0; i < numberVertices; i++)
    {
        memcpy(&spShape->vertices[i].position, &cubeVertices[i * 4], sizeof(glm::vec3));
        memcpy(&spShape->vertices[i].normal, &cubeNormals[i * 3], sizeof(glm::vec3));
        memcpy(&spShape->vertices[i].texcoord, &cubeTexCoords[i * 2], sizeof(glm::vec2));
        memcpy(&spShape->vertices[i].color, &color, sizeof(glm::vec4));
    }

    for (uint32_t i = 0; i < numberIndices; i++)
    {
        memcpy(&spShape->indices[i], &cubeIndices[i], sizeof(uint32_t));
    }

    for (uint32_t i = 0; i < numberVertices; i++)
    {
        spShape->vertices[i].position *= (sz * .5f);
        spShape->vertices[i].position += off;
    }

    return spShape;
}

} // MUtils
