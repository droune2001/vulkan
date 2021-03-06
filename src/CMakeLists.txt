cmake_minimum_required (VERSION 3.12)

set( VULKAN_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake" )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VULKAN_MODULE_PATH})
set( CMAKE_PREFIX_PATH ${VULKAN_MODULE_PATH})

# default compilation flags
set( CMAKE_USER_MAKE_RULES_OVERRIDE_CXX "${VULKAN_MODULE_PATH}/cxx_flag_overrides.cmake" )

project (Vulkan)

set( CMAKE_DEBUG_POSTFIX d )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# bin/lib dirs in each build dir.
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" ) # .exe
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" ) # .dll ou .so
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" ) # .lib windows, .a linux

set( ASSETS_DIR "${CMAKE_SOURCE_DIR}/../data")

# global includes for all projects
#include_directories(${CMAKE_BINARY_DIR})
include_directories("$ENV{GLM_DIR}")
include_directories("$ENV{VULKAN_SDK_DIR}/Include")

#add_subdirectory(tuto1-4)
#add_subdirectory(tuto5-12)
add_subdirectory(particles_loop)
set_property( TARGET particles_loop PROPERTY FOLDER "app" )
set_property( TARGET particles_loop_shaders PROPERTY FOLDER "shaders" )

# startup project for the whole solution, if it has never been opened (wont work if refreshing)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT particles_loop)

# $(VULKAN_SDK_DIR)\bin\glslangValidator.exe -V %(FullPath) -o %(RelativeDir)%(Filename)%(Extension).spv
# %(RelativeDir)%(Filename)%(Extension).spv

# libraries
# $(VULKAN_SDK_DIR)\Lib
# vulkan-1.lib

# add_definitions("-DAJALinux -DAJA_LINUX")

# install( DIRECTORY "${CMAKE_SOURCE_DIR}/Assets" DESTINATION . )
# install( FILES "${CMAKE_SOURCE_DIR}/scripts/solidtrack-linux.cfg" DESTINATION . RENAME "solidtrack.cfg" )