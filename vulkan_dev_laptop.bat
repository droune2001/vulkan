@echo off

echo Setup Visual Studio 2017 Environment Variables

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Set Vulkan Environment Variables
set VULKAN_SDK_VERSION=1.1.73.0
set VULKAN_SDK_DIR=C:\VulkanSDK\%VULKAN_SDK_VERSION%
set PATH=%VULKAN_SDK_DIR%\Bin;%PATH%
set VULKAN_PATH=%VULKAN_SDK_DIR%\Bin

echo Set Tools Environment Variables
set MY_CMAKE_PATH=C:\Users\Nicolas\home\bin\cmake-3.12.2-win64-x64\bin
set PATH=%MY_CMAKE_PATH%;%PATH%

echo Set Librairies Environment Variables
set GLM_DIR=C:\Users\Nicolas\home\dev\glm-0.9.9.0

cd C:\Users\Nicolas\home\dev\vulkan

doskey DEV=devenv vc2017\vulkan.sln
doskey cmaker=cmake -G"Visual Studio 15 2017 Win64" -Hsrc -Bbuild