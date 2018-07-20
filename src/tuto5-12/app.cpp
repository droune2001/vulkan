#include "app.h"

#include "build_options.h"
#include "platform.h"
#include "Shared.h" // Log
#include "Renderer.h"
#include "window.h"
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
    if (!_w) return false;

    _scene = new Scene();

    Scene::object_description_t obj_desc = {};
    _scene->add_object(obj_desc);

    return true;
}


bool VulkanApplication::loop()
{
    Log("# App::run()...\n");

    // FPS
    auto timer = std::chrono::steady_clock();
    auto last_time = timer.now();
    uint64_t frame_counter = 0;
    uint64_t fps = 0;

    while (_r->Run())
    {
        // CPU Logic calculations
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
        _r->Draw();
    }

    Log("#   Wait Queue Idle\n");
    vkQueueWaitIdle(_r->GetVulkanQueue());

    return true;
}

void VulkanApplication::clean()
{
    Log("# App::clean()\n");
    delete _r;
}
