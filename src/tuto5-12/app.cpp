#include "app.h"

#include "build_options.h"
#include "platform.h"
#include "Shared.h" // Log
#include "Renderer.h"
#include "window.h"
#include "utils.h"
#include "Scene.h"

#include <array>
#include <chrono>
#include <sstream>

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

    Log("#  Create/Init Renderer\n");
    _r = new Renderer();
    if (!_r->Init())
        return false;

    // TODO(nfauvet): window class that creates itself with a renderer.

    Log("#  Creating Window\n");
    Window *_w = _r->OpenWindow(800, 600, "test");
    if (!_w) 
        return false;

    _scene = new Scene(_r); // _r.context() ??

    IndexedMesh icosphere = make_icosphere(3); // 3 = 642 vtx, 1280 tri, 3840 idx
    Scene::object_description_t obj_desc = {};
    obj_desc.vertexCount = (uint32_t)icosphere.first.size();
    obj_desc.vertices    = icosphere.first.data();
    obj_desc.indexCount  = (uint32_t)icosphere.second.size();
    obj_desc.indices     = icosphere.second.data();

    _scene->add_object(obj_desc);

    return true;
}


bool VulkanApplication::loop()
{
    Log("# App::run()...\n");

    // FPS
    auto timer = std::chrono::steady_clock();
    auto last_time = timer.now();
    auto last_time_for_dt = last_time;
    uint64_t frame_counter = 0;
    uint64_t fps = 0;

    while (_r->Run())
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
        _r->Draw(dt, _scene);
    }

    Log("#   Wait Queue Idle\n");
    vkQueueWaitIdle(_r->GetVulkanQueue());

    return true;
}

void VulkanApplication::clean()
{
    Log("# App::clean()\n");
    delete _scene;
    delete _r;
}