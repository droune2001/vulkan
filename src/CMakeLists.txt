cmake_minimum_required (VERSION 3.12)
project (Vulkan)

#include_directories("${PROJECT_BINARY_DIR}")

# global includes for all projects
include_directories("$ENV{GLM_DIR}")

#add_subdirectory(tuto1-4)
#add_subdirectory(tuto5-12)
add_subdirectory(particles_loop)
