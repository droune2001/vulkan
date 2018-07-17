#pragma once

#include <string>
#include <vector>
#include <array>

class Renderer;

class Window
{
public:
    Window(Renderer *renderer, uint32_t size_x, uint32_t size_y, const std::string & title);
    ~Window();

    bool Init();
    void Close();
    bool Update();
    
    // can be put elsewhere. here because the window has access to the images/depth
    void BeginRender(VkSemaphore wait_semaphore);
    void EndRender( std::vector<VkSemaphore> wait_semaphores );

    VkRenderPass render_pass()                      { return _render_pass; }
    VkFramebuffer active_swapchain_framebuffer()    { return _swapchain_framebuffers[_active_swapchain_image_id]; }
    VkImage active_swapchain_image()                { return _swapchain_images[_active_swapchain_image_id]; }
    VkExtent2D surface_size()                       { return {_surface_size_x, _surface_size_y}; }
    VkImage depth_stencil_image()                   { return _depth_stencil_image; }
    VkPipeline pipeline(size_t i)                   { return _pipelines[i]; }
    VkPipelineLayout pipeline_layout()              { return _pipeline_layout; }
    VkDescriptorSet *descriptor_set_ptr()           { return &_descriptor_set; }
    VkBuffer *triangle_vbo_ptr()                    { return &(_triangle_vbo.buffer); }

    void set_camera_position(float, float, float);
    void update_matrices_ubo();

private:

    void InitOSWindow();
    void DeInitOSWindow();
    void UpdateOSWindow();

    bool InitOSSurface();
    bool InitSurface();
    void DeInitSurface();

    bool InitSwapChain();
    void DeInitSwapChain();

    bool InitSwapChainImages();
    void DeInitSwapChainImages();

    bool InitDepthStencilImage();
    void DeInitDepthStencilImage();

    bool InitRenderPass();
    void DeInitRenderPass();

    bool InitSwapChainFrameBuffers();
    void DeInitSwapChainFrameBuffers();

    bool InitSynchronizations();
    void DeInitSynchronizations();

    bool InitUniformBuffer();
    void DeInitUniformBuffer();

    bool InitDescriptors();
    void DeInitDescriptors();

    bool InitVertexBuffer();
    void DeInitVertexBuffer();

    bool InitFakeImage();
    void DeInitFakeImage();

    bool InitShaders();
    void DeInitShaders();

    bool InitGraphicsPipeline();
    void DeInitGraphicsPipeline();

    VkDevice device();

public:

    // ==== APPLICATION/MAIN ======
    bool _window_should_run = true;
    // ============================

    Renderer * _renderer = nullptr;
    std::string _window_name;
    
    // ==== WSI/WINDOW ============
    uint32_t _surface_size_x = 512;
    uint32_t _surface_size_y = 512;
    uint32_t _swapchain_image_count = 2;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR _surface_caps = {};
    VkSurfaceFormatKHR _surface_format = {};
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;
    std::vector<VkFramebuffer> _swapchain_framebuffers;
    uint32_t _active_swapchain_image_id = UINT32_MAX;
    VkFence _swapchain_image_available_fence = VK_NULL_HANDLE;
    // ============================
    

    VkImage _depth_stencil_image = {};
    VkDeviceMemory _depth_stencil_image_memory = VK_NULL_HANDLE;
    VkImageView _depth_stencil_image_view = VK_NULL_HANDLE;
    VkFormat _depth_stencil_format = VK_FORMAT_UNDEFINED;
    bool _stencil_available = false;

    VkRenderPass _render_pass = VK_NULL_HANDLE;

    // ==== OBJECT =========
    struct vertex
    {
        float x, y, z, w;
        float nx, ny, nz;
        float u, v;
    };

    struct vertex_buffer
    {
        VkBuffer        buffer = VK_NULL_HANDLE;
        VkDeviceMemory  memory = VK_NULL_HANDLE;
    } _triangle_vbo;
    // =====================

	VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSet _descriptor_set = VK_NULL_HANDLE;
	
    // ==== MATERIAL =======
    VkShaderModule _vertex_shader_module = VK_NULL_HANDLE;
    VkShaderModule _fragment_shader_module = VK_NULL_HANDLE;

    std::array<VkPipeline, 1> _pipelines = {};
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    // =====================


    
    struct matrices
    {
        float m[16];
        float v[16];
        float p[16];
    } _mvp;

    struct uniform_buffer
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    } _matrices_ubo;

    struct texture
    {
        VkImage         texture_image        = VK_NULL_HANDLE;
        VkDeviceMemory  texture_image_memory = {};
        VkImageView     texture_view         = VK_NULL_HANDLE;
        VkSampler       sampler              = VK_NULL_HANDLE;
    } _checker_texture;

public:

#ifdef VK_USE_PLATFORM_WIN32_KHR
    HWND _win32_window = 0;
    HINSTANCE _win32_instance = 0;
    std::string _win32_class_name = {};
    static uint64_t _win32_class_id_counter;
#endif
};
