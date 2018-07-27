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
IndexedMesh make_flat_cube();
IndexedMesh make_cube();

//
// SHADER UTILS
//

std::vector<char> ReadFileContent(const std::string &file_path);

#endif // !_VULKAN_UTILS_2018_07_18_H_
