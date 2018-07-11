@echo off

echo Setup Visual Studio 2017 Environment Variables

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Add Vulkan to environment PATH

set PATH=C:\VulkanSDK\1.1.73.0\Bin;%PATH%

echo Set VULKAN_PATH

set VULKAN_PATH=C:\VulkanSDK\1.1.73.0\Bin

cd C:\Users\Nicolas\home\dev\vulkan

doskey DEV=devenv vc2017\vulkan.sln