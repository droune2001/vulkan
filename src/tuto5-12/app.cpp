#include "build_options.h"
#include "platform.h"

#include "app.h"
#include "Shared.h" // Log
#include "window.h"

#include "Renderer.h"
#include "utils.h"
#include "Scene.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

#include "glm_usage.h"

#include <array>
#include <chrono>
#include <sstream>
#include <random>
#include <functional>

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
    if (!_w->OpenWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "vulkan testbed"))
        return false;

    Log("#----------------------------------------\n");
    Log("#  Create Renderer/Init Context\n");
    _r = new Renderer(_w);
    if (!_r->InitContext())
        return false;

    Log("#----------------------------------------\n");
    Log("#  Create ImGUI Context\n");
    if (!InitImGui())
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
    bool should_refresh_fps = false;
    uint64_t frame_counter = 0;
    uint64_t fps = 0;

    bool show_demo_window = true;

    while (_w->Update())
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // CPU Logic calculations
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(timer.now() - last_time_for_dt);
        last_time_for_dt = timer.now();
        float dt = duration.count() / 1000000.0f;

        should_refresh_fps = false;
        ++frame_counter;
        if (last_time + std::chrono::seconds(1) < timer.now())
        {
            last_time = timer.now();
            fps = frame_counter;
            frame_counter = 0;
            should_refresh_fps = true;

            //std::ostringstream oss;
            //oss << "*** FPS: " << fps << std::endl;
            //Log(oss.str().c_str());
        }

        ShowMainMenuBar();
        ShowFPSWindow(should_refresh_fps, fps);

        //if (show_demo_window)
        //ImGui::ShowDemoWindow(&show_demo_window);

        _r->Update(dt);

        ImGui::Render();

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

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    Log("#  Destroy Context\n");
    delete _r;

    Log("#  Destroy Window\n");
    _w->DeleteWindow();
    delete _w;
}

void VulkanApplication::BuildScene()
{
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto real_rand = std::bind(std::uniform_real_distribution<float>(0, 1), std::mt19937((unsigned int)seed));

    _scene = new Scene(_r->context());
    _scene->init(_r->render_pass());

    //
    // Lights
    //

    Scene::light_description_t light_0;
    light_0.position = glm::vec3(0, 0, 0);
    light_0.color = glm::vec3(1, 1, 1);
    light_0.radius = 25.0f;
    light_0.intensity = 5.0f;
    _scene->add_light(light_0);

    Scene::light_description_t light_1;
    light_1.type = Scene::light_description_t::CONE_LIGHT_TYPE;
    light_1.position = glm::vec3(0, 2, 0);
    light_1.color = glm::vec3(1, 0, 1);
    light_1.radius = 25.0f;
    light_1.intensity = 5.0f;
    light_1.direction = glm::normalize(glm::vec3(0.2f, -1, 0.3f));
    light_1.inner = PI_5;
    light_1.outer = PI_4;
    _scene->add_light(light_1);

    float l_min = 0.2f;
    float l_scale = (1.0f - l_min);
    for (int i = 2; i < 8; i++)
    {
        Scene::light_description_t light;
        light.position = glm::vec3(0, 0, 0);
        float r = l_min + l_scale * real_rand();
        float g = l_min + l_scale * real_rand();
        float b = l_min + l_scale * real_rand();
        light.color = glm::vec3(r, g, b);
        light.radius = 25.0f;
        light.intensity = 4.0f;
        _scene->add_light(light);
    }

    //
    // Cameras
    //

    Scene::camera_description_t camera = {};
    camera.camera_id = "perspective";
    camera.position = glm::vec3(10, 0, 0);
    camera.near_plane = 0.1f;
    camera.far_plane = 20.0f;
    camera.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    camera.fovy = 45.0f;
    _scene->add_camera(camera);

    //
    // Materials
    //
    {
        Scene::material_instance_description_t mi = {};
        mi.instance_id = "neutral_dielectric";
        mi.pipeline_id = "default";
        mi.base_tex = "neutral_base";
        mi.specular_tex = "neutral_dielectric_spec";
        _scene->add_material_instance(mi);
    }

    {
        Scene::material_instance_description_t mi = {};
        mi.instance_id = "half_metal_checker";
        mi.pipeline_id = "default";
        mi.base_tex = "checker_base";
        mi.specular_tex = "checker_spec";
        _scene->add_material_instance(mi);
    }

    {
        Scene::material_instance_description_t mi = {};
        mi.instance_id = "neutral_metal";
        mi.pipeline_id = "default";
        mi.base_tex = "neutral_base";
        mi.specular_tex = "neutral_metal_spec";
        _scene->add_material_instance(mi);
    }



    //
    // Objects
    //
#define WITH_WALLS 1
#define WITH_CUBES 0
#define WITH_INSTANCED_CUBES 1

#define NB_SPHERES 10
    // SPHERE - shiny red plastic
    for (size_t i = 0; i < NB_SPHERES; ++i)
    {
        float ith = (float)i / (NB_SPHERES - 1);
        IndexedMesh icosphere = make_icosphere(3, 0.5f); // 3 = 642 vtx, 1280 tri, 3840 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.name = std::string("DielectricSphere_") + std::to_string(i);
        obj_desc.vertexCount = (uint32_t)icosphere.first.size();
        obj_desc.vertices = icosphere.first.data();
        obj_desc.indexCount = (uint32_t)icosphere.second.size();
        obj_desc.indices = icosphere.second.data();
        obj_desc.position = glm::vec3(-4.5f + 9.0f*ith, 0.0f, -1.0f);
        obj_desc.material = "neutral_dielectric";
        obj_desc.base_color = glm::vec4(0.97, 0.74, 0.62, 1); // copper tint
        obj_desc.specular = glm::vec4(0.045f + 0.955f*ith, 0, 0.5f, 0);
        _scene->add_object(obj_desc);
    }

    // SPHERE - rough green metal
    for (size_t i = 0; i < NB_SPHERES; ++i)
    {
        float ith = (float)i / (NB_SPHERES - 1);
        IndexedMesh icosphere = make_icosphere(3, 0.5f); // 3 = 642 vtx, 1280 tri, 3840 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.name = std::string("MetalSphere_") + std::to_string(i);
        obj_desc.vertexCount = (uint32_t)icosphere.first.size();
        obj_desc.vertices = icosphere.first.data();
        obj_desc.indexCount = (uint32_t)icosphere.second.size();
        obj_desc.indices = icosphere.second.data();
        obj_desc.position = glm::vec3(-4.5f + 9.0f*ith, 0.0f, 1.0f);
        obj_desc.material = "neutral_metal";
        obj_desc.base_color = glm::vec4(0.97, 0.74, 0.62, 1); // copper
        obj_desc.specular = glm::vec4(0.045f + 0.955f*ith, 1, 1, 0);
        _scene->add_object(obj_desc);
    }

#if WITH_WALLS == 1

    // FLOOR
    {
        IndexedMesh obj = make_flat_cube(20.0f, 1.0f, 20.0f); // 24 vtx, 12 tri, 36 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(0.0f, -2.0f, 0.0f);
        obj_desc.material = "half_metal_checker";
        obj_desc.base_color = glm::vec4(1, 1, 1, 1); // no modification
        obj_desc.specular = glm::vec4(1, 1, 0, 0); // no modification
        _scene->add_object(obj_desc);
    }
#if 0
    // LEFT WALL
    {
        IndexedMesh obj = make_flat_cube(1.0f, 10.0f, 10.0f); // 24 vtx, 12 tri, 36 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(-5.5f, 0.0f, 0.0f);
        obj_desc.material = "half_metal_checker";
        obj_desc.base_color = glm::vec4(1, 1, 1, 1); // no modification
        obj_desc.specular = glm::vec4(1, 1, 0, 0); // no modification
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
        obj_desc.position = glm::vec3(5.5f, 0.0f, 0.0f);
        obj_desc.material = "half_metal_checker";
        obj_desc.base_color = glm::vec4(1, 1, 1, 1); // no modification
        obj_desc.specular = glm::vec4(1, 1, 0, 0); // no modification
        _scene->add_object(obj_desc);
    }

    // FAR WALL
    {
        IndexedMesh obj = make_flat_cube(10.0f, 10.0f, 1.0f); // 24 vtx, 12 tri, 36 idx
        Scene::object_description_t obj_desc = {};
        obj_desc.name = std::string("FarWall");
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(0.0f, 0.0f, -5.5f);
        obj_desc.material = "half_metal_checker";
        obj_desc.base_color = glm::vec4(1, 1, 1, 1); // no modification
        obj_desc.specular = glm::vec4(1, 1, 0.5, 0); // common reflectance for dielectric cells
        _scene->add_object(obj_desc);
    }
#endif
#endif

#if WITH_CUBES == 1
    // metal [170..255]
    constexpr float metal_min = 170.0f / 255.0f;
    constexpr float metal_scale = (255.0f - 170.0f) / 255.0f;

    // dielectrics [50..240]
    constexpr float dielectric_min = 50.0f / 255.0f;
    constexpr float dielectric_max = 240.0f / 255.0f;
    constexpr float dielectric_scale = dielectric_max - dielectric_min;

    for (uint32_t i = 0; i < 19; ++i)
    {
        for (uint32_t j = 0; j < 19; ++j)
        {
            //auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            //auto real_rand = std::bind(std::uniform_real_distribution<float>(0, 1), std::mt19937((unsigned int)seed));

            IndexedMesh obj = make_flat_cube(0.5f, 0.5f, 0.5f);
            Scene::object_description_t obj_desc = {};
            obj_desc.name = std::string("Cube_") + std::to_string(i * 19 + j);
            obj_desc.vertexCount = (uint32_t)obj.first.size();
            obj_desc.vertices = obj.first.data();
            obj_desc.indexCount = (uint32_t)obj.second.size();
            obj_desc.indices = obj.second.data();
            //obj_desc.position = glm::vec3(-4.5f+i*0.5f, -5.0f+0.5f*real_rand(), -4.5f+j*0.5f);
            obj_desc.position = glm::vec3(-4.5f + i * 0.5f, -1.0f, -4.5f + j * 0.5f);
            //bool is_metal = (real_rand() > 0.5f); // binary random metallic
            bool is_metal = (i <j); // diagonal
            if (is_metal)
            {
                obj_desc.material = "neutral_metal";
                float roughness = 0.045f + 0.955f * real_rand(); // random roughness
                float metallic = 1.0f;
                float r = metal_min + metal_scale * real_rand();
                float g = metal_min + metal_scale * real_rand();
                float b = metal_min + metal_scale * real_rand();
                obj_desc.base_color = glm::vec4(r, g, b, 1); // random tint
                obj_desc.specular = glm::vec4(roughness, metallic, 1, 0);
            }
            else
            {
                obj_desc.material = "neutral_dielectric";
                float roughness = 0.045f + 0.955f * real_rand(); // random roughness
                float metallic = 0.0f;
                float r = dielectric_min + dielectric_scale * real_rand();
                float g = dielectric_min + dielectric_scale * real_rand();
                float b = dielectric_min + dielectric_scale * real_rand();
                obj_desc.base_color = glm::vec4(r, g, b, 1); // random tint
                obj_desc.specular = glm::vec4(roughness, metallic, 0.5f, 0);
            }
            _scene->add_object(obj_desc);
        }
    }
#endif

#if WITH_INSTANCED_CUBES == 1
    //
    // PLASTIC instance set
    //
    {
        IndexedMesh obj = make_flat_cube(0.5f, 0.5f, 0.5f);
        Scene::object_description_t obj_desc = {};
        obj_desc.name = std::string("Cube_Template");
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(0, 0, 0);
        obj_desc.material = "neutral_dielectric";
        obj_desc.base_color = glm::vec4(1, 1, 1, 1);
        obj_desc.specular = glm::vec4(0.1, 0, 0.5, 0);

        Scene::instance_set_description_t is_desc;
        is_desc.instance_set = "plastic_cubes";
        is_desc.object_desc = obj_desc;

        _scene->add_instance_set(is_desc);
    }

    //
    // METAL instance set
    //
    {
        IndexedMesh obj = make_flat_cube(0.5f, 0.5f, 0.5f);
        Scene::object_description_t obj_desc = {};
        obj_desc.name = std::string("Cube_Template");
        obj_desc.vertexCount = (uint32_t)obj.first.size();
        obj_desc.vertices = obj.first.data();
        obj_desc.indexCount = (uint32_t)obj.second.size();
        obj_desc.indices = obj.second.data();
        obj_desc.position = glm::vec3(0, 0, 0);
        obj_desc.material = "neutral_metal";
        obj_desc.base_color = glm::vec4(1, 1, 1, 1);
        obj_desc.specular = glm::vec4(0.1, 1, 0.5, 0);

        Scene::instance_set_description_t is_desc;
        is_desc.instance_set = "metal_cubes";
        is_desc.object_desc = obj_desc;

        _scene->add_instance_set(is_desc);
    }

    // metal [170..255]
    constexpr float metal_min = 170.0f / 255.0f;
    constexpr float metal_scale = (255.0f - 170.0f) / 255.0f;

    // dielectrics [50..240]
    constexpr float dielectric_min = 50.0f / 255.0f;
    constexpr float dielectric_max = 240.0f / 255.0f;
    constexpr float dielectric_scale = dielectric_max - dielectric_min;

    const int ROWS_COUNT = 16;
    const int COLS_COUNT = 16;

    for (uint32_t i = 0; i < ROWS_COUNT; ++i)
    {
        for (uint32_t j = 0; j < COLS_COUNT; ++j)
        {
            Scene::instanced_object_description_t instanced_object_desc;
            instanced_object_desc.position = glm::vec3(-4.5f + i * 0.5f, -1.0f, -4.5f + j * 0.5f);
            instanced_object_desc.rotation = glm::vec3(0, 0, 0);
            instanced_object_desc.scale = glm::vec3(1, 1, 1);
            
            float roughness = 0.045f + 0.955f * real_rand(); // random roughness
            float metallic = 0.0f;
            float r = dielectric_min + dielectric_scale * real_rand();
            float g = dielectric_min + dielectric_scale * real_rand();
            float b = dielectric_min + dielectric_scale * real_rand();
            instanced_object_desc.base_color = glm::vec4(r, g, b, 1); // random tint
            instanced_object_desc.specular = glm::vec4(roughness, metallic, 0.5f, 0);

            _scene->add_object_to_instance_set(instanced_object_desc, "plastic_cubes");
        }
    }
#endif

    _scene->compile();
}

//
// IMGUI
//

bool VulkanApplication::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    //(void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplWin32_Init(_w->_win32_window); // hwnd

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = _r->context()->instance;
    init_info.PhysicalDevice = _r->context()->physical_device;
    init_info.Device = _r->context()->device;
    init_info.QueueFamily = _r->context()->graphics.family_index;
    init_info.Queue = _r->context()->graphics.queue;
    init_info.PipelineCache = nullptr;// g_PipelineCache;
    init_info.DescriptorPool = _r->context()->descriptor_pool;
    init_info.Allocator = nullptr;// g_Allocator;
    init_info.CheckVkResultFn = _ErrorCheck;
    ImGui_ImplVulkan_Init(&init_info, _r->render_pass());

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    io.Fonts->AddFontDefault();

    return true;
}

void VulkanApplication::ShowMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ShowMenuFile();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void VulkanApplication::ShowMenuFile()
{
    ImGui::MenuItem("(dummy menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {}
    if (ImGui::MenuItem("Open", "Ctrl+O")) {}
    if (ImGui::BeginMenu("Open Recent"))
    {
        ImGui::MenuItem("fish_hat.c");
        ImGui::MenuItem("fish_hat.inl");
        ImGui::MenuItem("fish_hat.h");
        if (ImGui::BeginMenu("More.."))
        {
            ImGui::MenuItem("Hello");
            ImGui::MenuItem("Sailor");
            if (ImGui::BeginMenu("Recurse.."))
            {
                ShowMenuFile();
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {}
    if (ImGui::MenuItem("Save As..")) {}
    ImGui::Separator();
    if (ImGui::BeginMenu("Options"))
    {
        static bool enabled = true;
        ImGui::MenuItem("Enabled", "", &enabled);
        ImGui::BeginChild("child", ImVec2(0, 60), true);
        for (int i = 0; i < 10; i++)
            ImGui::Text("Scrolling Text %d", i);
        ImGui::EndChild();
        static float f = 0.5f;
        static int n = 0;
        static bool b = true;
        ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
        ImGui::InputFloat("Input", &f, 0.1f);
        ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
        ImGui::Checkbox("Check", &b);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Colors"))
    {
        float sz = ImGui::GetTextLineHeight();
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImGui::GetColorU32((ImGuiCol)i));
            ImGui::Dummy(ImVec2(sz, sz));
            ImGui::SameLine();
            ImGui::MenuItem(name);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Disabled", false)) // Disabled
    {
        IM_ASSERT(0);
    }
    if (ImGui::MenuItem("Checked", NULL, true)) {}
    if (ImGui::MenuItem("Quit", "Alt+F4")) {}
}

void VulkanApplication::ShowFPSWindow(bool should_refresh_fps, uint64_t fps)
{
    ImGuiWindowFlags window_flags = 0;
    //window_flags |= ImGuiWindowFlags_MenuBar;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoNav;
    bool *pOpen = nullptr;

    ImGui::Begin("FPS", pOpen, window_flags);

    static float values[30] = { 0 };
    static int values_offset = 0;
    static uint64_t max_fps = 1;
    if (should_refresh_fps)
    {
        values[values_offset] = (float)fps;
        values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
        if (fps > max_fps)
            max_fps = fps;
    }
    std::string text = std::string("FPS: ") + std::to_string(fps);
    ImGui::PlotLines("", values, IM_ARRAYSIZE(values), values_offset, text.c_str(), 0.0f, (float)max_fps, ImVec2(0, 50));
    //ImGui::PlotHistogram("", values, IM_ARRAYSIZE(values), 0, text.c_str(), 0.0f, (float)max_fps, ImVec2(0, 50));

    ImGui::End();
}

//
