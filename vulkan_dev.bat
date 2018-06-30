@echo off

echo Setup Visual Studio 2017 Environment Variables

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Add Vulkan and 4coder to environment PATH

set PATH=C:\Users\Nicolas\home\bin\4coder;C:\VulkanSDK\1.1.73.0\Bin;%PATH%

echo Set VULKAN_PATH

set VULKAN_PATH=C:\VulkanSDK\1.1.73.0\Bin

cd C:\Users\Nicolas\home\dev\vulkan