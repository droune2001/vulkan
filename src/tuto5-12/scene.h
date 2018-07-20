#ifndef _VULKAN_SCENE_2018_07_20_H_
#define _VULKAN_SCENE_2018_07_20_H_

#define MAX_OBJECTS 1024

class Scene
{
public:
    struct object_description_t
    {
    
    };

    void add_object(object_description_t od);

private:
    struct _object_t
    {};

    _object_t _objects[MAX_OBJECTS];
    uint32_t  _nb_objects = 0;
};

#endif // _VULKAN_SCENE_2018_07_20_H_
