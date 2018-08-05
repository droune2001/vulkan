@echo off
echo "Compiling shaders"
pushd data
glslangvalidator.exe -V simple.vert -o simple.vert.spv
glslangvalidator.exe -V simple.frag -o simple.frag.spv
popd
