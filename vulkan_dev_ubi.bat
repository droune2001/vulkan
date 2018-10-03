@echo off

echo Setup Visual Studio 2015 Environment Variables

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

echo Set Vulkan Environment Variables
set VULKAN_SDK_VERSION=1.1.82.1
set VULKAN_SDK_DIR=C:\VulkanSDK\%VULKAN_SDK_VERSION%
set PATH=%VULKAN_SDK_DIR%\Bin;%PATH%
set VULKAN_PATH=%VULKAN_SDK_DIR%\Bin

echo Set Tools Environment Variables
set MY_CMAKE_PATH=D:\dev\bin\cmake-3.12.2-win64-x64\bin
set PATH=%MY_CMAKE_PATH%;%PATH%

echo Set Librairies Environment Variables
set GLM_DIR=D:\dev\glm-0.9.9.0

D:

cd D:\dev\vulkan

REM doskey DEV=devenv vc2015\Vulkan.sln
doskey dev=devenv build\vulkan.sln
doskey cmaker=cmake -G"Visual Studio 14 2015 Win64" -Hsrc -Bbuild
