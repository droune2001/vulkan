#ifndef _VULKAN_SCENE_2018_07_20_H_
#define _VULKAN_SCENE_2018_07_20_H_

//#include "build_options.h"
//#include "platform.h"

#include <stdint.h> // uint33_t
#include <glm/glm.hpp> // glm::vec3

#include <array>

#define MAX_OBJECTS 1024
class Renderer;

class Scene
{
public:
    Scene(Renderer *);
    ~Scene();

    struct vertex_t
    {
        glm::vec4 p;
        glm::vec3 n;
        glm::vec2 uv;
    };
    using index_t = uint16_t;

    struct object_description_t
    {
        uint32_t indexCount = 0;
        uint16_t *indices = nullptr;
        uint32_t vertexCount = 0;
        vertex_t *vertices = nullptr;
    };

    bool add_object(object_description_t od);

    uint32_t objectCount() const { return _nb_objects; }
    void draw_object(uint32_t object_index, VkCommandBuffer cmd);
    void draw_all_objects(VkCommandBuffer cmd);

private:

    Renderer *_r = nullptr;

    struct _object_t
    {
        uint32_t vertexCount = 0;
        VkBuffer index_buffer = VK_NULL_HANDLE;
        VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;

        uint32_t indexCount = 0;
        VkBuffer vertex_buffer = VK_NULL_HANDLE;
        VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
    };

    std::array<_object_t, MAX_OBJECTS> _objects;
    uint32_t _nb_objects = 0;
};

#endif // _VULKAN_SCENE_2018_07_20_H_
