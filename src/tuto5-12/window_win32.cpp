#include "build_options.h"
#include "platform.h"
#include "window.h"
#include "Renderer.h"
#include "Shared.h"

#include <assert.h>

uint64_t Window::_win32_class_id_counter = 0;

LRESULT CALLBACK WindowsEventHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window *window = reinterpret_cast<Window*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg)
    {
    case WM_CLOSE:
        window->Close();
        // return 0;
        ::PostQuitMessage(0);
        break;

    default: break;
    }

    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Window::InitOSWindow()
{
    WNDCLASSEX win_class;
    assert(_surface_size.width > 0 && _surface_size.height > 0);

    _win32_instance = ::GetModuleHandle(nullptr);
    _win32_class_name = _window_name + std::to_string(_win32_class_id_counter);
    _win32_class_id_counter++;

    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_VREDRAW | CS_HREDRAW; // CS_OWNDC ??
    win_class.lpfnWndProc = WindowsEventHandler;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = _win32_instance; // hInstance
    win_class.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = _win32_class_name.c_str();
    win_class.hIconSm = ::LoadIcon(NULL, IDI_WINLOGO);

    if (!::RegisterClassEx(&win_class))
    {
        assert(!"Cannnot create window");
        std::exit(-1);
    }

    DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT wr = { 0, 0, LONG(_surface_size.width), LONG(_surface_size.height)};
    ::AdjustWindowRectEx(&wr, style, FALSE, ex_style);
    _win32_window = ::CreateWindowEx(
        0,
        _win32_class_name.c_str(),
        _window_name.c_str(),
        style,
        0,0,//CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left,
        wr.bottom - wr.top,
        NULL,
        NULL,
        _win32_instance,
        NULL);
    // strore our Window object pointer into the user data of the created win32 window. 
    ::SetWindowLongPtr(_win32_window, GWLP_USERDATA, (LONG_PTR)this);
    ::ShowWindow(_win32_window, SW_SHOW);
    ::SetForegroundWindow(_win32_window);
    ::SetFocus(_win32_window);
}

void Window::DeInitOSWindow()
{
    ::DestroyWindow(_win32_window);
    ::UnregisterClass(_win32_class_name.c_str(), _win32_instance);
}

void Window::UpdateOSWindow()
{
    MSG msg;
    if (::PeekMessage(&msg, _win32_window, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

bool Window::InitOSSurface()
{
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.hinstance = _win32_instance;
    surface_create_info.hwnd = _win32_window;
    
    auto result = vkCreateWin32SurfaceKHR(_ctx->instance, &surface_create_info, nullptr, &_surface);
    ErrorCheck(result);

    return (result == VK_SUCCESS);
}
