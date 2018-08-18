#ifndef _VULKAN_UTILS_2018_07_18_H_
#define _VULKAN_UTILS_2018_07_18_H_

#include "build_options.h"
#include "platform.h"
#include "scene.h" // vertext_t, index_t

#include "glm_usage.h"

#include <vector>

//
// MESH UTILS
//

struct triangle_t
{
    Scene::index_t vertex_index[3];
};

using TriangleList = std::vector<triangle_t>;
using IndexList = std::vector<Scene::index_t>;
using VertexList = std::vector<Scene::vertex_t>;
using IndexedMesh = std::pair<VertexList, IndexList>;

IndexedMesh make_icosphere(int subdivisions);
IndexedMesh make_flat_cube(float width = 1.0f, float height = 1.0f, float depth = 1.0f);

namespace utils
{

    //
    // SHADER UTILS
    //

    std::vector<char> read_file_content(const std::string &file_path);

    //
    // OTHER
    //

    void* aligned_alloc(size_t size, size_t alignment);
    void aligned_free(void* data);

    struct loaded_image
    {
        uint32_t width;
        uint32_t height;
        uint32_t size;
        void *data;
    };

    void create_checker_base_image(loaded_image *);
    void create_checker_spec_image(loaded_image *);
    void create_neutral_base_image(loaded_image *);
    void create_neutral_metal_spec_image(loaded_image *);
    void create_neutral_dielectric_spec_image(loaded_image *);

} // namespace utils

#endif // !_VULKAN_UTILS_2018_07_18_H_
