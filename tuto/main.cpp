#include "build_options.h"
#include "platform.h"
#include "Renderer.h"
#include "window.h"
#include "Shared.h"

#include <cstdio>

int main(int argc, char **argv)
{
	Log("### Main program starting.\n");

	{
		Log("# Creating Renderer\n");
		Renderer r;

		Log("# Init Renderer\n");
		if (!r.Init()) return -1;

		Log("# Creating Window\n");
		if (!r.OpenWindow(800, 600, "test")) return -1;

		Log("# Run...\n");
		while (r.Run())
		{

		}
	}

	Log("### DONE - Press any key...\n");
	getchar();
	return 0;
}
