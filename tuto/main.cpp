#include "platform.h"
#include "Renderer.h"

int main( int argc, char **argv )
{
    Renderer r;
	if (!r.Init())
		return -1;

	// continue Tutorial 5 at 13:49

    return 0;
}
