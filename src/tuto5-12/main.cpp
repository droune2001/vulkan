#include "app.h"

#include "build_options.h"
#include "platform.h"
#include "Shared.h" // Log

#include <cstdio> // getchar

int main(int argc, char **argv)
{
    Log("### Main program starting.\n");
    
    VulkanApplication app;
    app.run();

    Log("### DONE - Press any key...\n");
    getchar();
    return 0;
}
