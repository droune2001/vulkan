@echo off
echo "Compiling shaders"
pushd data

glslangvalidator.exe -V simple.vert -o simple.vert.spv
glslangvalidator.exe -V simple.frag -o simple.frag.spv

glslangvalidator.exe -V instancing.vert -o instancing.vert.spv
glslangvalidator.exe -V instancing.frag -o instancing.frag.spv

popd
