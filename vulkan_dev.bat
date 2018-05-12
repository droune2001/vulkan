@echo off

REM Setup Visual Studio 2017 env

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

PATH=C:\Users\Nicolas\home\bin\4coder;C:\VulkanSDK\1.1.73.0\Bin;%PATH%

cd C:\Users\Nicolas\home\dev\vulkan