#include "utils.h"
#include "Shared.h"

#include <map>
#include <array>
#include <fstream>

//
// ICO SPHERE
//

namespace icosahedron
{
    const float X = .525731112119133606f;
    const float Z = .850650808352039932f;
    const float N = 0.f;

    static const VertexList vertices =
    {
        { { -X, N, Z, 1 },{ -X, N, Z },{ 0,0 } },
        { { X, N, Z, 1 },{ X, N, Z },{ 0,0 } },
        { { -X, N,-Z, 1 },{ -X, N,-Z },{ 0,0 } },
        { { X, N,-Z, 1 },{ X, N,-Z },{ 0,0 } },
        { { N, Z, X, 1 },{ N, Z, X },{ 0,0 } },
        { { N, Z,-X, 1 },{ N, Z,-X },{ 0,0 } },
        { { N,-Z, X, 1 },{ N,-Z, X },{ 0,0 } },
        { { N,-Z,-X, 1 },{ N,-Z,-X },{ 0,0 } },
        { { Z, X, N, 1 },{ Z, X, N },{ 0,0 } },
        { { -Z, X, N, 1 },{ -Z, X, N },{ 0,0 } },
        { { Z,-X, N, 1 },{ Z,-X, N },{ 0,0 } },
        { { -Z,-X, N, 1 },{ -Z,-X, N },{ 0,0 } },
    };

    static const TriangleList triangles =
    {
        { 0,4,1 },{ 0,9,4 },{ 9,5,4 },{ 4,5,8 },{ 4,8,1 },
        { 8,10,1 },{ 8,3,10 },{ 5,3,8 },{ 5,2,3 },{ 2,7,3 },
        { 7,10,3 },{ 7,6,10 },{ 7,11,6 },{ 11,0,6 },{ 0,1,6 },
        { 6,1,10 },{ 9,0,11 },{ 9,11,2 },{ 9,2,5 },{ 7,2,11 }
    };
}

using Lookup = std::map<std::pair<Scene::index_t, Scene::index_t>, Scene::index_t>;

Scene::index_t vertex_for_edge(
    Lookup& lookup,
    VertexList& vertices, 
    Scene::index_t first, Scene::index_t second)
{
    Lookup::key_type key(first, second);
    if (key.first > key.second)
        std::swap(key.first, key.second);

    auto inserted = lookup.insert({key, (Scene::index_t)vertices.size()});
    if (inserted.second)
    {
        const glm::vec4& edge0 = vertices[first].p;
        const glm::vec4& edge1 = vertices[second].p;
        Scene::vertex_t v = {};
        v.p = glm::vec4(glm::normalize(glm::vec3(edge0) + glm::vec3(edge1)), 1.0f);
        v.n = v.p; // meme chose, normale = position model space
        v.uv = {0.0f, 0.0f}; // TODO: long-lat conversion of pos
        vertices.push_back(v);
    }

    return inserted.first->second;
}

TriangleList subdivide(
    VertexList& vertices,
    TriangleList triangles)
{
    Lookup lookup;
    TriangleList result;

    for (auto&& each : triangles)
    {
        std::array<Scene::index_t, 3> mid;
        for (int edge = 0; edge < 3; ++edge)
        {
            mid[edge] = vertex_for_edge(lookup, vertices,
                each.vertex_index[edge], each.vertex_index[(edge + 1) % 3]);
        }

        result.push_back({each.vertex_index[0], mid[0], mid[2]});
        result.push_back({each.vertex_index[1], mid[1], mid[0]});
        result.push_back({each.vertex_index[2], mid[2], mid[1]});
        result.push_back({mid[0], mid[1], mid[2]});
    }

    return result;
}

IndexedMesh make_icosphere(int subdivisions)
{
    VertexList vertices = icosahedron::vertices;
    TriangleList triangles = icosahedron::triangles;

    for (int i = 0; i < subdivisions; ++i)
    {
        triangles = subdivide(vertices, triangles);
    }

    IndexList indices;
    indices.resize(triangles.size()*3);
    for (auto t : triangles)
    {
        indices.push_back(t.vertex_index[0]);
        indices.push_back(t.vertex_index[1]);
        indices.push_back(t.vertex_index[2]);
    }
    return{vertices, indices};
}

namespace flat_cube
{
    static const VertexList vertices =
    {
        // +Z, CCW looking at center, starting at -1,-1,1
        { { -1, -1,  1, 1 },{ 0, 0, 1 },{ 0, 1 } },
        { {  1, -1,  1, 1 },{ 0, 0, 1 },{ 1, 1 } },
        { {  1,  1,  1, 1 },{ 0, 0, 1 },{ 1, 0 } },
        { { -1,  1,  1, 1 },{ 0, 0, 1 },{ 0, 0 } },
        // -Z, CCW looking at center, starting at 1,-1,-1
        { {  1, -1, -1, 1 },{ 0, 0, -1 },{ 0, 1 } },
        { { -1, -1, -1, 1 },{ 0, 0, -1 },{ 1, 1 } },
        { { -1,  1, -1, 1 },{ 0, 0, -1 },{ 1, 0 } },
        { {  1,  1, -1, 1 },{ 0, 0, -1 },{ 0, 0 } },
        // +X, CCW looking at center, starting at 1,-1,1
        { {  1, -1,  1, 1 },{ 1, 0, 0 },{ 0, 1 } },
        { {  1, -1, -1, 1 },{ 1, 0, 0 },{ 1, 1 } },
        { {  1,  1, -1, 1 },{ 1, 0, 0 },{ 1, 0 } },
        { {  1,  1,  1, 1 },{ 1, 0, 0 },{ 0, 0 } },
        // -X, CCW looking at center, starting at 1,-1,1
        { { -1, -1, -1, 1 },{ -1, 0, 0 },{ 0, 1 } },
        { { -1, -1,  1, 1 },{ -1, 0, 0 },{ 1, 1 } },
        { { -1,  1,  1, 1 },{ -1, 0, 0 },{ 1, 0 } },
        { { -1,  1, -1, 1 },{ -1, 0, 0 },{ 0, 0 } },
        // +Y, CCW looking at center, starting at -1,1,1
        { { -1,  1,  1, 1 },{ 0, 1, 0 },{ 0, 1 } },
        { {  1,  1,  1, 1 },{ 0, 1, 0 },{ 1, 1 } },
        { {  1,  1, -1, 1 },{ 0, 1, 0 },{ 1, 0 } },
        { { -1,  1, -1, 1 },{ 0, 1, 0 },{ 0, 0 } },
        // -Y, CCW looking at center, starting at -1,-1,-1
        { { -1, -1, -1, 1 },{ 0, -1, 0 },{ 0, 1 } },
        { {  1, -1, -1, 1 },{ 0, -1, 0 },{ 1, 1 } },
        { {  1, -1,  1, 1 },{ 0, -1, 0 },{ 1, 0 } },
        { { -1, -1,  1, 1 },{ 0, -1, 0 },{ 0, 0 } },
    };

    static const TriangleList triangles =
    {
        { 0,1,2 },{ 0,2,3 }, // CCW +Z
        { 4,5,6 },{ 4,6,7 }, // CCW -Z
        { 8,9,10 },{ 8,10,11 },
        { 12,13,14 },{ 12,14,15 },
        { 16,17,18 },{ 16,18,19 },
        { 20,21,22 },{ 20,22,23 }
    };
}

IndexedMesh make_flat_cube(float width, float height, float depth)
{
    VertexList vertices;
    vertices.reserve(flat_cube::vertices.size());
    for (auto v : flat_cube::vertices)
    {
        Scene::vertex_t scaled_v = v;
        scaled_v.p *= glm::vec4(0.5f*width, 0.5f*height, 0.5f*depth, 1.0f);
        vertices.push_back(scaled_v);
    }
    TriangleList triangles = flat_cube::triangles;

    IndexList indices;
    indices.resize(triangles.size() * 3);
    for (auto t : triangles)
    {
        indices.push_back(t.vertex_index[0]);
        indices.push_back(t.vertex_index[1]);
        indices.push_back(t.vertex_index[2]);
    }

    return {vertices, indices};
}

//
// CUBE
//

// TODO
namespace cube
{
    static const VertexList vertices =
    {
        // +Z, CCW looking at center, starting at -1,-1
        { { -1, -1,  1, 1 },{ 0, 0, 1 },{ 0, 1 } },
        { { 1, -1,  1, 1 },{ 0, 0, 1 },{ 1, 1 } },
        { { 1,  1,  1, 1 },{ 0, 0, 1 },{ 1, 0 } },
        { { -1,  1,  1, 1 },{ 0, 0, 1 },{ 0, 0 } },
        // -Z, CCW looking at center, starting at 1,-1
        { { 1, -1, -1, 1 },{ 0, 0, -1 },{ 0, 1 } },
        { { -1, -1, -1, 1 },{ 0, 0, -1 },{ 1, 1 } },
        { { -1,  1, -1, 1 },{ 0, 0, -1 },{ 1, 0 } },
        { { 1,  1, -1, 1 },{ 0, 0, -1 },{ 0, 0 } },
    };

    static const TriangleList triangles =
    {
        { 0,1,2 },{ 0,2,3 }, // CCW +Z
    { 4,5,6 },{ 4,6,7 }, // CCW -Z
    { 8,9,10 },{ 8,10,11 },
    { 12,13,14 },{ 12,14,15 },
    { 16,17,18 },{ 16,18,19 },
    { 20,21,22 },{ 20,22,23 }
    };
}

IndexedMesh make_cube()
{
    VertexList vertices = flat_cube::vertices;
    TriangleList triangles = flat_cube::triangles;

    IndexList indices;
    indices.resize(triangles.size() * 3);
    for (auto t : triangles)
    {
        indices.push_back(t.vertex_index[0]);
        indices.push_back(t.vertex_index[1]);
        indices.push_back(t.vertex_index[2]);
    }

    return { vertices, indices };
}


namespace utils
{
    std::vector<char> read_file_content(const std::string &file_path)
    {
        std::vector<char> content;

        std::ifstream file(file_path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            Log("!!!!!!!!! Failed to open shader file.\n");
            return std::vector<char>();
        }
        else
        {
            size_t file_size = (size_t)file.tellg();
            std::vector<char> buffer(file_size);

            file.seekg(0);
            file.read(buffer.data(), file_size);

            file.close();
            return buffer;
        }
    }

    void* aligned_alloc(size_t size, size_t alignment)
    {
        void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
        data = _aligned_malloc(size, alignment);
#else 
        int res = posix_memalign(&data, alignment, size);
        if (res != 0)
            data = nullptr;
#endif
        return data;
    }

    void aligned_free(void* data)
    {
#if	defined(_MSC_VER) || defined(__MINGW32__)
        _aligned_free(data);
#else 
        free(data);
#endif
    }

    void create_checker_image(loaded_image *checker_image)
    {
        checker_image->width = 512;
        checker_image->height = 512;
        checker_image->size = sizeof(float) * checker_image->width * checker_image->height * 3;
        checker_image->data = (void *) new float[checker_image->width * checker_image->height * 3];

        for (uint32_t x = 0; x < checker_image->width; ++x)
        {
            for (uint32_t y = 0; y < checker_image->height; ++y)
            {
                float g = 0.3f;
                if (x % 40 < 20 && y % 40 < 20) { g = 1; }
                if (x % 40 >= 20 && y % 40 >= 20) { g = 1; }

                float *pixel = ((float *)checker_image->data) + (x * checker_image->height * 3) + (y * 3);
                pixel[0] = g * 1.0f;// 0.4f;
                pixel[1] = g * 1.0f;// 0.5f;
                pixel[2] = g * 1.0f;// 0.7f;
            }
        }
    }

    void create_default_image(loaded_image *default_image)
    {
        default_image->width = 2;
        default_image->height = 2;
        default_image->size = sizeof(uint8_t) * default_image->width * default_image->height * 4;
        default_image->data = (void *) new uint8_t[default_image->width * default_image->height * 4];

        for (uint32_t x = 0; x < default_image->width; ++x)
        {
            for (uint32_t y = 0; y < default_image->height; ++y)
            {
                uint8_t *pixel = ((uint8_t *)default_image->data) + 4 * (x * default_image->height + y);
                pixel[0] = 187;
                pixel[1] = 187;
                pixel[2] = 187;
                pixel[3] = 255;

            }
        }
    }
} // namespace utils

//
