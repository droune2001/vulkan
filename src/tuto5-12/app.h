#ifndef _VULKAN_APPLICATION_2018_07_20_H_
#define _VULKAN_APPLICATION_2018_07_20_H_

//
// BASE APPLICATION
//

class BaseApplication
{
public:
    void run();

protected:
    virtual bool init()  = 0;
    virtual bool loop()  = 0;
    virtual void clean() = 0;
};

//
// VULKAN APPLICATION
//

class Renderer;
class Window;
class Scene;

class VulkanApplication : public BaseApplication
{
public:
    VulkanApplication();
    ~VulkanApplication();

protected:
    virtual bool init() override;
    virtual bool loop() override;
    virtual void clean() override;

private:
    // context_t *_context = nullptr;
    // wsi_t     *_wsi = nullptr;

    Renderer * _r = nullptr;
    Window   * _w = nullptr;
    Scene    * _scene = nullptr;
};

#endif //
