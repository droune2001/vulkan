#include "build_options.h"
#include "platform.h"

#include "app.h"
#include "Shared.h" // Log
#include "window.h"

#include "Renderer.h"
#include "utils.h"
#include "Scene.h"

#include "glm_usage.h"

#include <array>
#include <chrono>
#include <sstream>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

void BaseApplication::run()
{
    if (!init()) return;
    loop();
    clean();
}

VulkanApplication::VulkanApplication() : BaseApplication()
{
}

VulkanApplication::~VulkanApplication()
{
}

bool VulkanApplication::init()
{
    Log("# App::init()\n");

    Log("#  Creating Window\n");
    _w = new Window();
    if (!_w->OpenWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "test"))
        return false;

    Log("#----------------------------------------\n");
    Log("#  Create Renderer/Init Context\n");
    _r = new Renderer(_w);
    if (!_r->InitContext())
        return false;

    Log("#----------------------------------------\n");
    Log("#  Init Scene\n");
    BuildScene();
    
    _r->SetScene(_scene);

    return true;
}

bool VulkanApplication::loop()
{
    Log("#----------------------------------------\n");
    Log("# App::run()...\n");

    // FPS
    auto timer = std::chrono::steady_clock();
    auto last_time = timer.now();
    auto last_time_for_dt = last_time;
    uint64_t frame_counter = 0;
    uint64_t fps = 0;

    while (_w->Update())
    {
        // CPU Logic calculations
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - last_time_for_dt);
        last_time_for_dt = timer.now();
        float dt = duration.count() / 1000.0f;

        ++frame_counter;
        if (last_time + std::chrono::seconds(1) < timer.now())
        {
            last_time = timer.now();
            fps = frame_counter;
            frame_counter = 0;

            std::ostringstream oss;
            oss << "*** FPS: " << fps << std::endl;
            Log(oss.str().c_str());
        }

        // Actual drawing
        _r->Draw(dt);
    }

    Log("#----------------------------------------\n");
    Log("#   Wait Queue Idle\n");
    vkQueueWaitIdle(_r->context()->graphics.queue);

    return true;
}

void VulkanApplication::clean()
{
    Log("# App::clean()\n");

    Log("#  Destroy Scene\n");
    delete _scene;

    Log("#  Destroy Context\n");
    delete _r;

    Log("#  Destroy Window\n");
    _w->DeleteWindow();
    delete _w;
}

void VulkanApplication::BuildScene()
{
    _scene = new Scene(_r->context());
    _scene->init(_r->render_pass());

    Scene::light_description_t light = {};
    light.color = glm::vec3(1,0.5,1);
    light.position = glm::vec3(4,4,2);
    _scene->add_light(light);

    Scene::camera_description_t camera = {};
    camera.position = glm::vec3(10, 0, 0);
    camera.near_plane = 0.1f;
    camera.far_plane = 20.0f;
    camera.aspect = (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT;
    camera.fovy = 45.0f;
    _scene->add_camera(camera);

    {
        IndexedMesh icosphere = make_icosphere(3); // 3 = 642 vtx, 1280 tri, 3840 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.vertexCount = (uint32_t)icosphere.first.size();
        obj_desc.vertices = icosphere.first.data();
        obj_desc.indexCount = (uint32_t)icosphere.second.size();
        obj_desc.indices = icosphere.second.data();
        obj_desc.position = glm::vec3(-2.5f, 0.0f, -1.0f);
        obj_desc.material = "default";
        obj_desc.diffuse_texture = "checker";
        _scene->add_object(obj_desc);
    }

    {
        IndexedMesh obj = make_flat_cube(); // 24 vtx, 12 tri, 36 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(2.5f, 0.0f, 1.0f);
        obj_desc.material = "default";
        obj_desc.diffuse_texture = "checker";
        _scene->add_object(obj_desc);
    }

    // FLOOR
    {
        IndexedMesh obj = make_flat_cube(10.0f, 1.0f, 10.0f); // 24 vtx, 12 tri, 36 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(0.0f, -5.0f, 0.0f);
        obj_desc.material = "default";
        obj_desc.diffuse_texture = "checker";
        _scene->add_object(obj_desc);
    }

    // LEFT WALL
    {
        IndexedMesh obj = make_flat_cube(1.0f, 10.0f, 10.0f); // 24 vtx, 12 tri, 36 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(-5.0f, 0.0f, 0.0f);
        obj_desc.material = "default";
        obj_desc.diffuse_texture = "checker";
        _scene->add_object(obj_desc);
    }

    // RIGHT WALL
    {
        IndexedMesh obj = make_flat_cube(1.0f, 10.0f, 10.0f); // 24 vtx, 12 tri, 36 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(5.0f, 0.0f, 0.0f);
        obj_desc.material = "default";
        obj_desc.diffuse_texture = "checker";
        _scene->add_object(obj_desc);
    }

    // FAR WALL
    {
        IndexedMesh obj = make_flat_cube(10.0f, 10.0f, 1.0f); // 24 vtx, 12 tri, 36 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(0.0f, 0.0f, -5.0f);
        obj_desc.material = "default";
        obj_desc.diffuse_texture = "checker";
        _scene->add_object(obj_desc);
    }

    for (uint32_t i = 0; i < 19; ++i)
    {
        for (uint32_t j = 0; j < 19; ++j)
        {
            IndexedMesh obj = make_flat_cube(0.2f, 0.2f, 0.2f);//make_icosphere(2);
            Scene::object_description_t obj_desc = {};
            obj_desc.vertexCount = (uint32_t)obj.first.size();
            obj_desc.vertices = obj.first.data();
            obj_desc.indexCount = (uint32_t)obj.second.size();
            obj_desc.indices = obj.second.data();
            obj_desc.position = glm::vec3(-4.0f+i*0.5f, -4.0f, -4.0f+j*0.5f);
            obj_desc.material = "default";
            obj_desc.diffuse_texture = "checker";
            _scene->add_object(obj_desc);
        }
    }

}
//
