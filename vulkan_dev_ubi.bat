@echo off

echo Setup Visual Studio 2015 Environment Variables

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

echo Set Vulkan Environment Variables
set VULKAN_SDK_VERSION=1.1.77.0
set VULKAN_SDK_DIR=C:\VulkanSDK\%VULKAN_SDK_VERSION%
set PATH=%VULKAN_SDK_DIR%\Bin;%PATH%
set VULKAN_PATH=%VULKAN_SDK_DIR%\Bin

echo Set Librairies Environment Variables
set GLM_DIR=D:\dev\glm-0.9.9.0

D:

cd D:\dev\vulkan

doskey DEV=devenv vc2015\vulkan.sln
