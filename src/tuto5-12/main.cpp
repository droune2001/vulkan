#include "build_options.h"
#include "platform.h"
#include "Renderer.h"
#include "window.h"
#include "Shared.h"

#include <cstdio>
#include <array>
#include <chrono>
#include <sstream>



class VulkanApplication
{
public:
    void run();

private:
    bool init();
    bool loop();
    void clean();

    Renderer *_r = nullptr;
    Window   *_w = nullptr;
};

bool VulkanApplication::init()
{
    Log("# Create/Init Renderer\n");
    _r = new Renderer();
    if (!_r->Init())
        return false;

    // TODO(nfauvet): window class that creates itself with a renderer.

    Log("# Creating Window\n");
    Window *_w = _r->OpenWindow(800, 600, "test");
    if (!_w) return false;

    return true;
}

void VulkanApplication::run()
{
    if (!init())
        return;

    // FPS
    auto timer = std::chrono::steady_clock();
    auto last_time = timer.now();
    uint64_t frame_counter = 0;
    uint64_t fps = 0;

    Log("# Run...\n");
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

    Log("#  Wait Queue Idle\n");
    vkQueueWaitIdle(_r->GetVulkanQueue());

    clean();
}

void VulkanApplication::clean()
{
    delete _r;
}


int main(int argc, char **argv)
{
    Log("### Main program starting.\n");
    
    VulkanApplication app;
    app.run();

    Log("### DONE - Press any key...\n");
    getchar();
    return 0;
}
