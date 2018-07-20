@echo off
echo "Compiling shaders"
pushd data
glslangvalidator.exe -V simple.vert
glslangvalidator.exe -V simple.frag
popd
