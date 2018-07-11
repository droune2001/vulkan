@echo off

echo Setup Visual Studio 2015 Environment Variables

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

echo Add Vulkan environment PATH

set PATH=C:\VulkanSDK\1.1.77.0\Bin;%PATH%

echo Set VULKAN_PATH

set VULKAN_PATH=C:\VulkanSDK\1.1.77.0\Bin

D:

cd D:\dev\vulkan

doskey DEV=devenv vc2015\vulkan.sln
