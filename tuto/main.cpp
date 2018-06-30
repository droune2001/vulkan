#include "Renderer.h"

int main( int argc, char **argv )
{
    Renderer r;
	if (!r.Init())
		return -1;

    return 0;
}
