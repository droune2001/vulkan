
add_executable(particles_loop main.cpp)

install (TARGETS particles_loop DESTINATION bin)

# to copy SPIR-V files??
#install (FILES MathFunctions.h DESTINATION include)