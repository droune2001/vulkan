#pragma once

#include <string>
#include <vector>
#include <array>

struct vulkan_context;

class Window
{
public:
    Window();
    ~Window();

    // creates an os specific window.
    bool OpenWindow(uint32_t size_x, uint32_t size_y, const std::string &title);
    // deletes the os specific window.
    void DeleteWindow();
    // init the window specific vulkan parts (swapchain surface/surface views)
    bool InitVulkanWindowSpecifics(vulkan_context *c);
    // releases the window specific vulkan parts (swapchain surface/surface views)
    bool DeInitVulkanWindowSpecifics(vulkan_context *c);
    // polls window events
    bool Update();
    // closes the window and frees vulkan resources
    void Close();
    // Acquire Next Swapchain Image
    void BeginRender(VkSemaphore wait_semaphore);
    // Present Swapchain Image to queue
    void EndRender( std::vector<VkSemaphore> wait_semaphores );

    uint32_t swapchain_image_count()            { return _swapchain_image_count; }
    VkFormat surface_format()                   { return _surface_format.format; }
    uint32_t active_swapchain_image_id()        { return _active_swapchain_image_id; }
    VkImage active_swapchain_image()            { return _swapchain_images[_active_swapchain_image_id]; }
    VkImageView swapchain_image_views(size_t i) { return _swapchain_image_views[i]; }
    VkExtent2D surface_size()                   { return _surface_size; }

private:
    
    void InitOSWindow();
    void DeInitOSWindow();
    void UpdateOSWindow();

    bool InitOSSurface();
    bool InitSurface();
        VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surface_formats);
        VkExtent2D ChooseSurfaceExtent();
    void DeInitSurface();

    bool InitSwapChain();
        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> &present_modes);
    void DeInitSwapChain();

    bool InitSwapChainImages();
    void DeInitSwapChainImages();

    VkDevice device();

public:

#ifdef VK_USE_PLATFORM_WIN32_KHR
    HWND _win32_window = 0;
    HINSTANCE _win32_instance = 0;
    std::string _win32_class_name = {};
    static uint64_t _win32_class_id_counter;
#endif

    bool _window_should_run = true;
    vulkan_context * _ctx = nullptr;
    
    // ==== WSI/WINDOW ============
    std::string _window_name;
    VkExtent2D _surface_size = { 512,512 };
    uint32_t _swapchain_image_count = 3;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR _surface_caps = {};
    VkSurfaceFormatKHR _surface_format = {};
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;
    uint32_t _active_swapchain_image_id = UINT32_MAX;
    // ============================
};
