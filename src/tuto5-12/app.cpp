#include "app.h"

#include "build_options.h"
#include "platform.h"
#include "Shared.h" // Log
#include "Renderer.h"
#include "window.h"

#include <array>
#include <chrono>
#include <sstream>

bool VulkanApplication::init()
{
    Log("#  Create/Init Renderer\n");
    _r = new Renderer();
    if (!_r->Init())
        return false;

    // TODO(nfauvet): window class that creates itself with a renderer.

    Log("#  Creating Window\n");
    Window *_w = _r->OpenWindow(800, 600, "test");
    if (!_w) return false;

    return true;
}

void VulkanApplication::run()
{
    Log("# App::init()\n");
    if (!init())
        return;

    Log("# App::run()...\n");
    loop();

    Log("# App::clean()\n");
    clean();
}

bool VulkanApplication::loop()
{
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
    delete _r;
}
