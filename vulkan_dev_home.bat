@echo off

echo Setup Visual Studio 2017 Environment Variables

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Set VULKAN env vars

REM use VK_SDK_PATH if existing

set VULKAN_SDK_VERSION=1.1.82.1
set VULKAN_SDK_DIR=C:\VulkanSDK\%VULKAN_SDK_VERSION%
set PATH=%VULKAN_SDK_DIR%\Bin;%PATH%
set VULKAN_PATH=%VULKAN_SDK_DIR%\Bin

echo Set Tools Environment Variables
set MY_CMAKE_PATH=C:\home\dev\cmake-3.12.2-win64-x64\bin
set PATH=%MY_CMAKE_PATH%;%PATH%

echo Set Librairies Environment Variables
set GLM_DIR=C:\home\dev\glm-0.9.9.0

cd C:\home\dev\vulkan

doskey DEV=devenv vc2017_home\vulkan.sln
doskey cmaker=cmake -G"Visual Studio 15 2017 Win64" -Hsrc -Bbuild_test