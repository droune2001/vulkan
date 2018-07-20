#ifndef _VULKAN_APPLICATION_2018_07_20_H_
#define _VULKAN_APPLICATION_2018_07_20_H_

class Renderer;
class Window;

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

#endif //
