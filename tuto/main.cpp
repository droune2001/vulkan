#include "build_options.h"
#include "platform.h"
#include "Renderer.h"
#include "window.h"

#if VULKAN_CONSOLE_APP == 1

int main( int argc, char **argv )
{
    Renderer r;
	if (!r.Init())
		return -1;

	// continue Tutorial 5 at 13:49

    return 0;
}

#elif VULKAN_WINDOWS_APP_V1 == 1

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	switch (uMsg) 
	{
	case WM_CLOSE:
		::PostQuitMessage(0); 
		break;

	default: break;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = L"VulkanWindowClass";
	::RegisterClassEx(&windowClass);

	HWND windowHandle = ::CreateWindowEx(NULL, L"VulkanWindowClass", L"Core",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		100,
		100,
		800,    // some random values for now. 
		600,    // we will come back to these soon.
		NULL,
		NULL,
		hInstance,
		NULL);

	Renderer r;
	if (!r.Init(hInstance, windowHandle))
	{
		return EXIT_FAILURE;
	}





	MSG msg;
	bool done = false;
	while (!done) 
	{
		::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);
		if (msg.message == WM_QUIT) 
		{
			done = true;
		}
		else 
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		::RedrawWindow(windowHandle, NULL, NULL, RDW_INTERNALPAINT);
	}

	return msg.wParam;
}

#elif VULKAN_WINDOWS_APP_V2 == 1

int main(int argc, char **argv)
{
	Renderer r;
	
	if (!r.Init()) return -1;
	
	r.OpenWindow(800, 600, "test");
	
	while(r.Run())
	{

	}

	return 0;
}

#else

#endif
