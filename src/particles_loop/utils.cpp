#include "utils.h"
#include "Shared.h"

#include <map>
#include <array>
#include <fstream>
#include <chrono>
#include <random>
#include <functional>

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

IndexedMesh make_icosphere(int subdivisions, float radius)
{
    VertexList vertices = icosahedron::vertices;
    TriangleList triangles = icosahedron::triangles;

    for (int i = 0; i < subdivisions; ++i)
    {
        triangles = subdivide(vertices, triangles);
    }

#ifndef PI
#define PI 3.14159f
#endif
    for (auto & vertex : vertices)
    {
        glm::vec3 unit = glm::normalize(vertex.p.xyz());
        float u = (std::atan2(unit.x, std::fabsf(unit.z)) + PI) / PI * 0.5f;
        float v = (std::acos(unit.y) + PI) / PI - 1.0f;
        vertex.p = glm::vec4(radius*vertex.p.xyz(), 1.0f);
        vertex.uv = { u, v };
    }

    IndexList indices;
    indices.reserve(triangles.size()*3);
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
    indices.reserve(triangles.size() * 3);
    for (auto t : triangles)
    {
        indices.push_back(t.vertex_index[0]);
        indices.push_back(t.vertex_index[1]);
        indices.push_back(t.vertex_index[2]);
    }

    return {vertices, indices};
}

namespace hexagon
{

#define COS_30 0.866f
#define SIN_30 0.5f

    static const VertexList vertices =
    {
        // starting at top, clock-wise
        {{ 0,       0,      0, 1},{0, 0, 1},{0, 1}},
        {{ 0,       1,      0, 1},{0, 0, 1},{0, 1}},
        {{ COS_30,  SIN_30, 0, 1},{0, 0, 1},{0, 1}},
        {{ COS_30, -SIN_30, 0, 1},{0, 0, 1},{0, 1}},
        {{ 0,      -1,      0, 1},{0, 0, 1},{0, 1}},
        {{-COS_30, -SIN_30, 0, 1},{0, 0, 1},{0, 1}},
        {{-COS_30,  SIN_30, 0, 1},{0, 0, 1},{0, 1}}
    };

    static const TriangleList triangles =
    {
        {0,1,2},{0,2,3},{0,3,4},
        {0,4,5},{0,5,6},{0,6,1}
    };

    //enum { X_POS, X_NEG, Y_POS, Y_NEG, Z_POS, Z_NEG };
}

IndexedMesh make_hexagon(float width, float height, glm::vec3 normal)
{
    VertexList vertices;
    vertices.reserve(hexagon::vertices.size());
    for (auto v : hexagon::vertices)
    {
        Scene::vertex_t scaled_v = v;
        scaled_v.p *= glm::vec4(0.5f*width, 0.5f*height, 1.0f, 1.0f);
        scaled_v.n = normal;
        vertices.push_back(scaled_v);
    }
    TriangleList triangles = hexagon::triangles;

    IndexList indices;
    indices.reserve(triangles.size() * 3);
    for (auto t : triangles)
    {
        indices.push_back(t.vertex_index[0]);
        indices.push_back(t.vertex_index[1]);
        indices.push_back(t.vertex_index[2]);
    }

    return{vertices, indices};
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

    void create_checker_base_image(loaded_image *checker_image)
    {
        checker_image->width = 512;
        checker_image->height = 512;
        checker_image->size = sizeof(float) * checker_image->width * checker_image->height * 3;
        checker_image->data = (void *) new float[checker_image->width * checker_image->height * 3];

        // metal [170..255]
        constexpr float metal_min = 170.0f / 255.0f;
        constexpr float metal_scale = (255.0f - 170.0f) / 255.0f;

        // dielectrics [50..240]
        constexpr float dielectric_min = 50.0f / 255.0f;
        constexpr float dielectric_max = 240.0f / 255.0f;
        constexpr float dielectric_scale = dielectric_max - dielectric_min;

        for (uint32_t y = 0; y < checker_image->height; ++y)
        {
            float dy = (float)y / (float)checker_image->height;

            for (uint32_t x = 0; x < checker_image->width; ++x)
            {
                float dx = (float)x / (float)checker_image->width; 

                float *pixel = ((float *)checker_image->data) + 3 * (y * checker_image->width + x);
        
                float r = (1.0f - dx);
                float g = (dx) * (1.0f - dy);
                float b = dx * dy;

                if ((x % 40 < 20 && y % 40 < 20)
                || (x % 40 >= 20 && y % 40 >= 20)) 
                { 
                    pixel[0] = metal_min + metal_scale * r;
                    pixel[1] = metal_min + metal_scale * g;
                    pixel[2] = metal_min + metal_scale * b;
                }
                else
                {
                    // dielectric [10..240]
                    pixel[0] = dielectric_min + dielectric_scale * r;
                    pixel[1] = dielectric_min + dielectric_scale * g;
                    pixel[2] = dielectric_min + dielectric_scale * b;
                }
            }
        }
    }

    void create_checker_spec_image(loaded_image *checker_image)
    {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto real_rand = std::bind(std::uniform_real_distribution<float>(0, 1), std::mt19937((unsigned int)seed));

        checker_image->width = 512;
        checker_image->height = 512;
        checker_image->size = sizeof(float) * checker_image->width * checker_image->height * 3;
        checker_image->data = (void *) new float[checker_image->width * checker_image->height * 3];

        for (uint32_t y = 0; y < checker_image->height; ++y)
        {
            float dy = (float)y / (float)checker_image->height;

            for (uint32_t x = 0; x < checker_image->width; ++x)
            {
                float dx = (float)x / (float)checker_image->width;

                float *pixel = ((float *)checker_image->data) + 3 * (y * checker_image->width + x);

                float r = 0.9f;
                float m = 0.0f;
                float s = 0.5f;
                if (x % 40 < 20 && y % 40 < 20)
                { 
                    float dx = (x % 20) / 20.0f;
                    float dy = (y % 20) / 20.0f;
                    r = 0.05f+0.6f*dx*dy + 0.1f*real_rand()*dx*dy; m = 1.0f; s = 1.0f;
                }
                if (x % 40 >= 20 && y % 40 >= 20) 
                { 
                    float dx = (x % 20) / 20.0f; dx -= 0.5f;
                    float dy = (y % 20) / 20.0f; dy -= 0.5f;
                    float r2 = dx * dx + dy * dy;
                    r = 0.05f + 0.6f * r2 + 0.1f*real_rand()*r2; m = 1.0f; s = 1.0f;
                }

                pixel[0] = r; // roughness
                pixel[1] = m; // metallic
                pixel[2] = s; // reflectance
            }
        }
    }

    void create_neutral_base_image(loaded_image *default_image)
    {
        default_image->width = 16;
        default_image->height = 16;
        default_image->size = sizeof(uint8_t) * default_image->width * default_image->height * 4;
        default_image->data = (void *) new uint8_t[default_image->width * default_image->height * 4];

        for (uint32_t x = 0; x < default_image->width; ++x)
        {
            for (uint32_t y = 0; y < default_image->height; ++y)
            {
                uint8_t *pixel = ((uint8_t *)default_image->data) + 4 * (x * default_image->height + y);
                pixel[0] = 255;
                pixel[1] = 255;
                pixel[2] = 255;
                pixel[3] = 255;
            }
        }
    }

    void create_neutral_dielectric_spec_image(loaded_image *image)
    {
        image->width = 16;
        image->height = 16;
        image->size = sizeof(float) * image->width * image->height * 4;
        image->data = (void *) new float[image->width * image->height * 4];

        for (uint32_t x = 0; x < image->width; ++x)
        {
            for (uint32_t y = 0; y < image->height; ++y)
            {
                float *pixel = ((float*)image->data) + 4 * (x * image->height + y);
                pixel[0] = 1.0f; // roughness
                pixel[1] = 0.0f; // metallic
                pixel[2] = 1.0f; // reflectance
                pixel[3] = 0;
            }
        }
    }

    void create_neutral_metal_spec_image(loaded_image *image)
    {
        image->width = 16;
        image->height = 16;
        image->size = sizeof(float) * image->width * image->height * 4;
        image->data = (void *) new float[image->width * image->height * 4];

        for (uint32_t x = 0; x < image->width; ++x)
        {
            for (uint32_t y = 0; y < image->height; ++y)
            {
                float *pixel = ((float*)image->data) + 4 * (x * image->height + y);
                pixel[0] = 1.0f; // roughness
                pixel[1] = 1.0f; // metallic
                pixel[2] = 1.0f; // reflectance
                pixel[3] = 0;
            }
        }
    }

    /* METAL REFLECTANCE COMMON VALUES
    Silver    0.97, 0.96, 0.91
    Aluminum  0.91, 0.92, 0.92
    Titanium  0.76, 0.73, 0.69
    Iron      0.77, 0.78, 0.78
    Platinum  0.83, 0.81, 0.78
    Gold      1.00, 0.85, 0.57
    Brass     0.98, 0.90, 0.59
    Copper    0.97, 0.74, 0.62

    minimum roughness = 0.045 to avoid aliasing



    color temperature:
    1,700-1,800     Match flame                     255 125   0
    1,850-1,930     Candle flame                    255 135   1
    2,000-3,000     Sun at sunrise/sunset           255 166  76
    2,500-2,900     Household tungsten lightbulb    255 176  94
    3,000           Tungsten lamp 1K                255 184 111
    3,200-3,500     Quartz lights                   255 191 123
    3,200-3,700     Fluorescent lights              255 191 123
    3,275           Tungsten lamp 2K                255 193 128
    3,380           Tungsten lamp 5K, 10K           255 196 134
    5,000-5,400     Sun at noon                     255 233 215
    5,500-6,500     Daylight (sun + sky)            255 243 241
    5,500-6,500     Sun through clouds/haze         255 243 241
    6,000-7,500     Overcast sky                    250 246 255
    6,500           RGB monitor white point         255 248 254
    7,000-8,000     Shaded areas outdoors           235 236 255
    8,000-10,000    Partly cloudy sky               214 224 255
    */
} // namespace utils

//
