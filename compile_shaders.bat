@echo off
echo "Compiling shaders"
pushd data

glslangvalidator.exe -V simple.vert -o simple.vert.spv
glslangvalidator.exe -V simple.frag -o simple.frag.spv

glslangvalidator.exe -V instancing.vert -o instancing.vert.spv
glslangvalidator.exe -V instancing.frag -o instancing.frag.spv

glslangvalidator.exe -V luminance.vert -o luminance.vert.spv
glslangvalidator.exe -V luminance.frag -o luminance.frag.spv

glslangvalidator.exe -V particles.comp -o particles.comp.spv

popd
