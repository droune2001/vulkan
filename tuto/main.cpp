#include "build_options.h"
#include "platform.h"
#include "Renderer.h"
#include "window.h"

int main(int argc, char **argv)
{
	Renderer r;
	
	if (!r.Init()) return -1;
	
	if (!r.OpenWindow(800, 600, "test")) return -1;
	
	while(r.Run())
	{

	}

	return 0;
}
