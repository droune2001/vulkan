#include "build_options.h"
#include "platform.h"
#include "Renderer.h"
#include "window.h"
#include "Shared.h"

#include <cstdio>
#include <array>
#include <chrono>
#include <sstream>

/* 
TODO:

*/

int main(int argc, char **argv)
{
    Log("### Main program starting.\n");
    {
        Log("# Creating Renderer\n");
        Renderer r;

        Log("# Init Renderer\n");
        if (!r.Init()) return -1;

        Log("# Creating Window\n");
        Window *w = r.OpenWindow(800, 600, "test");
        if (!w) return -1;

        // FPS
        auto timer = std::chrono::steady_clock();
        auto last_time = timer.now();
        uint64_t frame_counter = 0;
        uint64_t fps = 0;

        Log("# Run...\n");
        while (r.Run())
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
			r.Draw();
        }

        Log("#  Wait Queue Idle\n");
        vkQueueWaitIdle(r.GetVulkanQueue());
    }

    Log("### DONE - Press any key...\n");
    getchar();
    return 0;
}
