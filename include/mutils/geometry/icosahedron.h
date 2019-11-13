///////////////////////////////////////////////////////////////////////////////
// Icosahedron.cpp
// ===============
// Polyhedron with 12 vertices, 30 edges and 20 faces (triangles) for OpenGL
// If the radius is r, then the length of edge is (r / sin(2pi/5)).
//
// Vertices of icosahedron are constructed with spherical coords by aligning
// the north pole to (0,0,r) and the south pole to (0,0,-r). Other 10 vertices
// are computed by rotating 72 degree along y-axis at the elevation angle
// +/- 26.565 (=arctan(1/2)).
//
// The unwrapped (paper model) of icosahedron and texture map looks like;
// (S,0)  3S  5S  7S  9S
//    /\  /\  /\  /\  /\      : 1st row (5 triangles)       //
//   /__\/__\/__\/__\/__\                                   //
// T \  /\  /\  /\  /\  /\    : 2nd row (10 triangles)      //
//    \/__\/__\/__\/__\/__\                                 //
// 2T  \  /\  /\  /\  /\  /   : 3rd row (5 triangles)       //
//      \/  \/  \/  \/  \/                                  //
//      2S  4S  6S  8S  (10S,3T)
// where S = 186/2048 = 0.0908203
//       T = 322/1024 = 0.3144531
// If a texture size is 2048x1024, S=186, T=322
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2018-07-17
// UPDATED: 2018-07-31
// http://www.songho.ca/opengl/gl_sphere.html
// CM: Removed GL rendering/helpers; just need the sphere parts
///////////////////////////////////////////////////////////////////////////////


#include <cmath>
#include <cstdint>

namespace MUtils
{

class Icosahedron
{
public:
    Icosahedron(float radius)
        : interleavedStride(32)
    {
        setRadius(radius);
    }

    void setRadius(float radius)
    {
        this->radius = radius;
        this->edgeLength = radius / sinf(2 * 3.141592f / 5);
        if (vertices.size() <= 0)
            buildVertices();
        else
            updateRadius(); // update vertex positions only
    }

    void setEdgeLength(float edge)
    {
        this->edgeLength = edge;
        this->radius = edge * sinf(2 * 3.141592f / 5);
        updateRadius(); // update vertex positions only
    }

    void updateRadius()
    {
        float scale = sqrtf(radius * radius / (vertices[0] * vertices[0] + vertices[1] * vertices[1] + vertices[2] * vertices[2]));

        std::size_t i, j;
        std::size_t count = vertices.size();
        for (i = 0, j = 0; i < count; i += 3, j += 8)
        {
            vertices[i] *= scale;
            vertices[i + 1] *= scale;
            vertices[i + 2] *= scale;

            // for interleaved array
            interleavedVertices[j] *= scale;
            interleavedVertices[j + 1] *= scale;
            interleavedVertices[j + 2] *= scale;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // compute 12 vertices of icosahedron using spherical coordinates
    // The north pole is at (0, 0, r) and the south pole is at (0,0,-r).
    // 5 vertices are placed by rotating 72 deg at elevation 26.57 deg (=atan(1/2))
    // 5 vertices are placed by rotating 72 deg at elevation -26.57 deg
    ///////////////////////////////////////////////////////////////////////////////
    std::vector<float> computeVertices()
    {
        const float PI = 3.1415926f;
        const float H_ANGLE = PI / 180 * 72;    // 72 degree = 360 / 5
        const float V_ANGLE = atanf(1.0f / 2);  // elevation = 26.565 degree

        std::vector<float> vertices(12 * 3);    // 12 vertices
        int i1, i2;                             // indices
        float z, xy;                            // coords
        float hAngle1 = -PI / 2 - H_ANGLE / 2;  // start from -126 deg at 2nd row
        float hAngle2 = -PI / 2;                // start from -90 deg at 3rd row

        // the first top vertex (0, 0, r)
        vertices[0] = 0;
        vertices[1] = 0;
        vertices[2] = radius;

        // 10 vertices at 2nd and 3rd rows
        for (int i = 1; i <= 5; ++i)
        {
            i1 = i * 3;         // for 2nd row
            i2 = (i + 5) * 3;   // for 3rd row

            z = radius * sin(V_ANGLE);              // elevaton
            xy = radius * cos(V_ANGLE);

            vertices[i1] = xy * cosf(hAngle1);      // x
            vertices[i2] = xy * cosf(hAngle2);
            vertices[i1 + 1] = xy * sinf(hAngle1);  // y
            vertices[i2 + 1] = xy * sinf(hAngle2);
            vertices[i1 + 2] = z;                   // z
            vertices[i2 + 2] = -z;

            // next horizontal angles
            hAngle1 += H_ANGLE;
            hAngle2 += H_ANGLE;
        }

        // the last bottom vertex (0, 0, -r)
        i1 = 11 * 3;
        vertices[i1] = 0;
        vertices[i1 + 1] = 0;
        vertices[i1 + 2] = -radius;

        return vertices;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // generate vertices with flat shading
    // each triangle is independent (no shared vertices)
    // NOTE: The texture coords are offset in order to align coords to image pixels
    //    (S,0)  3S  5S  7S  (9S,0)
    //       /\  /\  /\  /\  /\             //
    //      /__\/__\/__\/__\/__\(10S,T)     //
    // (0,T)\  /\  /\  /\  /\  /\           //
    //       \/__\/__\/__\/__\/__\(11S,2T)  //
    //  (S,2T)\  /\  /\  /\  /\  /          //
    //         \/  \/  \/  \/  \/           //
    //    (2S,3T)  4S  6S  8S  (10S,3T)
    // where S = 186/2048 = 0.0908203
    //       T = 322/1024 = 0.3144531, If texture size is 2048x1024, S=186, T=322
    ///////////////////////////////////////////////////////////////////////////////
    void buildVertices()
    {
        const float S_STEP = 186 / 2048.0f;     // horizontal texture step
        const float T_STEP = 322 / 1024.0f;     // vertical texture step

        // compute 12 vertices of icosahedron
        std::vector<float> tmpVertices = computeVertices();

        // clear memory of prev arrays
        std::vector<float>().swap(vertices);
        std::vector<float>().swap(normals);
        std::vector<float>().swap(texCoords);
        std::vector<unsigned int>().swap(indices);
        std::vector<unsigned int>().swap(lineIndices);

        float* v0, * v1, * v2, * v3, * v4, * v11;                // vertex positions
        float n[3];                                         // face normal
        float t0[2], t1[2], t2[2], t3[2], t4[2], t11[2];    // texCoords
        unsigned int index = 0;

        // compute and add 20 tiangles
        v0 = &tmpVertices[0];       // 1st vertex
        v11 = &tmpVertices[11 * 3]; // 12th vertex
        for (int i = 1; i <= 5; ++i)
        {
            // 4 vertices in the 2nd row
            v1 = &tmpVertices[i * 3];
            if (i < 5)
                v2 = &tmpVertices[(i + 1) * 3];
            else
                v2 = &tmpVertices[3];

            v3 = &tmpVertices[(i + 5) * 3];
            if ((i + 5) < 10)
                v4 = &tmpVertices[(i + 6) * 3];
            else
                v4 = &tmpVertices[6 * 3];

            // texture coords
            t0[0] = (2 * i - 1) * S_STEP;   t0[1] = 0;
            t1[0] = (2 * i - 2) * S_STEP;   t1[1] = T_STEP;
            t2[0] = (2 * i - 0) * S_STEP;   t2[1] = T_STEP;
            t3[0] = (2 * i - 1) * S_STEP;   t3[1] = T_STEP * 2;
            t4[0] = (2 * i + 1) * S_STEP;   t4[1] = T_STEP * 2;
            t11[0] = 2 * i * S_STEP;         t11[1] = T_STEP * 3;

            // add a triangle in 1st row
            computeFaceNormal(v0, v1, v2, n);
            addVertices(v0, v1, v2);
            addNormals(n, n, n);
            addTexCoords(t0, t1, t2);
            addIndices(index, index + 1, index + 2);

            // add 2 triangles in 2nd row
            computeFaceNormal(v1, v3, v2, n);
            addVertices(v1, v3, v2);
            addNormals(n, n, n);
            addTexCoords(t1, t3, t2);
            addIndices(index + 3, index + 4, index + 5);

            computeFaceNormal(v2, v3, v4, n);
            addVertices(v2, v3, v4);
            addNormals(n, n, n);
            addTexCoords(t2, t3, t4);
            addIndices(index + 6, index + 7, index + 8);

            // add a triangle in 3rd row
            computeFaceNormal(v3, v11, v4, n);
            addVertices(v3, v11, v4);
            addNormals(n, n, n);
            addTexCoords(t3, t11, t4);
            addIndices(index + 9, index + 10, index + 11);

            // add 6 edge lines per iteration
            addLineIndices(index);

            // next index
            index += 12;
        }

        // generate interleaved vertex array as well
        buildInterleavedVertices();
    }

    void buildInterleavedVertices()
    {
        std::vector<float>().swap(interleavedVertices);

        std::size_t i, j;
        std::size_t count = vertices.size();
        for (i = 0, j = 0; i < count; i += 3, j += 2)
        {
            interleavedVertices.push_back(vertices[i]);
            interleavedVertices.push_back(vertices[i + 1]);
            interleavedVertices.push_back(vertices[i + 2]);

            interleavedVertices.push_back(normals[i]);
            interleavedVertices.push_back(normals[i + 1]);
            interleavedVertices.push_back(normals[i + 2]);

            interleavedVertices.push_back(texCoords[j]);
            interleavedVertices.push_back(texCoords[j + 1]);
        }
    }

    void addVertices(float v1[3], float v2[3], float v3[3])
    {
        vertices.push_back(v1[0]);  // x
        vertices.push_back(v1[1]);  // y
        vertices.push_back(v1[2]);  // z
        vertices.push_back(v2[0]);
        vertices.push_back(v2[1]);
        vertices.push_back(v2[2]);
        vertices.push_back(v3[0]);
        vertices.push_back(v3[1]);
        vertices.push_back(v3[2]);
    }

    void addNormals(float n1[3], float n2[3], float n3[3])
    {
        normals.push_back(n1[0]);  // nx
        normals.push_back(n1[1]);  // ny
        normals.push_back(n1[2]);  // nz
        normals.push_back(n2[0]);
        normals.push_back(n2[1]);
        normals.push_back(n2[2]);
        normals.push_back(n3[0]);
        normals.push_back(n3[1]);
        normals.push_back(n3[2]);
    }

    void addTexCoords(float t1[2], float t2[2], float t3[2])
    {
        texCoords.push_back(t1[0]); // s
        texCoords.push_back(t1[1]); // t
        texCoords.push_back(t2[0]);
        texCoords.push_back(t2[1]);
        texCoords.push_back(t3[0]);
        texCoords.push_back(t3[1]);
    }

    void addIndices(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i3);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // add 6 edge lines to array starting from param (i)
    //  /   /   /   /   /       : (i, i+1)                          //
    // /__ /__ /__ /__ /__                                          //
    // \  /\  /\  /\  /\  /     : (i+3, i+4), (i+3, i+5), (i+4, i+5)//
    //  \/__\/__\/__\/__\/__                                        //
    //   \   \   \   \   \      : (i+9,i+10), (i+9, i+11)           //
    //    \   \   \   \   \                                         //
    ///////////////////////////////////////////////////////////////////////////////
    void addLineIndices(unsigned int index)
    {
        lineIndices.push_back(index);       // (i, i+1)
        lineIndices.push_back(index + 1);
        lineIndices.push_back(index + 3);     // (i+3, i+4)
        lineIndices.push_back(index + 4);
        lineIndices.push_back(index + 3);     // (i+3, i+5)
        lineIndices.push_back(index + 5);
        lineIndices.push_back(index + 4);     // (i+4, i+5)
        lineIndices.push_back(index + 5);
        lineIndices.push_back(index + 9);     // (i+9, i+10)
        lineIndices.push_back(index + 10);
        lineIndices.push_back(index + 9);     // (i+9, i+11)
        lineIndices.push_back(index + 11);
    }


    // static functions ===========================================================
    ///////////////////////////////////////////////////////////////////////////////
    // return face normal of a triangle v1-v2-v3
    // if a triangle has no surface (normal length = 0), then return a zero vector
    ///////////////////////////////////////////////////////////////////////////////
    void computeFaceNormal(float v1[3], float v2[3], float v3[3], float n[3])
    {
        const float EPSILON = 0.000001f;

        // default return value (0, 0, 0)
        n[0] = n[1] = n[2] = 0;

        // find 2 edge vectors: v1-v2, v1-v3
        float ex1 = v2[0] - v1[0];
        float ey1 = v2[1] - v1[1];
        float ez1 = v2[2] - v1[2];
        float ex2 = v3[0] - v1[0];
        float ey2 = v3[1] - v1[1];
        float ez2 = v3[2] - v1[2];

        // cross product: e1 x e2
        float nx, ny, nz;
        nx = ey1 * ez2 - ez1 * ey2;
        ny = ez1 * ex2 - ex1 * ez2;
        nz = ex1 * ey2 - ey1 * ex2;

        // normalize only if the length is > 0
        float length = sqrtf(nx * nx + ny * ny + nz * nz);
        if (length > EPSILON)
        {
            // normalize
            float lengthInv = 1.0f / length;
            n[0] = nx * lengthInv;
            n[1] = ny * lengthInv;
            n[2] = nz * lengthInv;
        }
    }

    // getters/setters
    float getRadius() const { return radius; }
    float getEdgeLength() const { return edgeLength; }

    // for vertex data
    unsigned int getVertexCount() const { return (unsigned int)vertices.size() / 3; }
    unsigned int getNormalCount() const { return (unsigned int)normals.size() / 3; }
    unsigned int getTexCoordCount() const { return (unsigned int)texCoords.size() / 2; }
    unsigned int getIndexCount() const { return (unsigned int)indices.size(); }
    unsigned int getLineIndexCount() const { return (unsigned int)lineIndices.size(); }
    unsigned int getTriangleCount() const { return getIndexCount() / 3; }

    unsigned int getVertexSize() const { return (unsigned int)vertices.size() * sizeof(float); }   // # of bytes
    unsigned int getNormalSize() const { return (unsigned int)normals.size() * sizeof(float); }
    unsigned int getTexCoordSize() const { return (unsigned int)texCoords.size() * sizeof(float); }
    unsigned int getIndexSize() const { return (unsigned int)indices.size() * sizeof(unsigned int); }
    unsigned int getLineIndexSize() const { return (unsigned int)lineIndices.size() * sizeof(unsigned int); }

    const float* getVertices() const { return vertices.data(); }
    const float* getNormals() const { return normals.data(); }
    const float* getTexCoords() const { return texCoords.data(); }
    const unsigned int* getIndices() const { return indices.data(); }
    const unsigned int* getLineIndices() const { return lineIndices.data(); }

    // for interleaved vertices: V/N/T
    unsigned int getInterleavedVertexCount() const { return getVertexCount(); }    // # of vertices
    unsigned int getInterleavedVertexSize() const { return (unsigned int)interleavedVertices.size() * sizeof(unsigned int); }    // # of bytes
    int getInterleavedStride() const { return interleavedStride; }   // should be 32 bytes
    const float* getInterleavedVertices() const { return interleavedVertices.data(); }

private:
    // memeber vars
    float radius;                           // circumscribed radius
    float edgeLength;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> lineIndices;

    // interleaved
    std::vector<float> interleavedVertices;
    int interleavedStride;                  // # of bytes to hop to the next vertex (should be 32 bytes)

}; // Icosahedron

} // MUtils
