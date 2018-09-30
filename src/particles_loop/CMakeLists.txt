# TODO: find the var for "current sub directory"

set(SHADERS_DIR "${ASSETS_DIR}/particles_loop")

set(GLSL_VALIDATOR "$ENV{VULKAN_SDK_DIR}/Bin/glslangValidator.exe")

file( GLOB particles_loop_SOURCES "*.c*" )
file( GLOB particles_loop_HEADERS "*.h*" )
file( GLOB particles_loop_SHADERS 
   "${SHADERS_DIR}/*.vert"
   "${SHADERS_DIR}/*.frag"
   "${SHADERS_DIR}/*.comp")
MESSAGE( STATUS "${particles_loop_SHADERS}" )
   
source_group( "Sources" FILES ${particles_loop_SOURCES} )
source_group( "Headers" FILES ${particles_loop_HEADERS} )
source_group( "Shaders" FILES ${particles_loop_SHADERS} )

add_executable(particles_loop 
    ${particles_loop_SOURCES} 
    ${particles_loop_HEADERS} 
    ${particles_loop_SHADERS})

set( CMAKE_INCLUDE_CURRENT_DIR ON )

# target_link_libraries(particles_loop vulkan_lib)

# set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:atlthunk.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:msvcrt.lib /SAFESEH:NO")
# set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:atlthunk.lib /NODEFAULTLIB:libcmt.lib /SAFESEH:NO")

file( TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}" NATIVE_CMAKE_SOURCE_DIR ) # top level cmakelists.txt directory, -H
file( TO_NATIVE_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" NATIVE_CMAKE_RUNTIME_OUTPUT_DIRECTORY ) # top level of build directory + /bin = /build/bin
file( TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" NATIVE_CMAKE_CURRENT_BINARY_DIR ) # local per-project, /build/solidtrackqt

# use ${PROJECT_BINARY_DIR} instead of ${CMAKE_CURRENT_BINARY_DIR}?

foreach(GLSL ${particles_loop_SHADERS})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/data/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/data/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
	COMMENT "Compiling shader XXX")
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(particles_loop Shaders)

add_custom_command(
    TARGET particles_loop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:particles_loop>/data/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/data"
        "$<TARGET_FILE_DIR:particles_loop/data"
)

install(TARGETS particles_loop DESTINATION .)
install(DIRECTORY "${SHADERS_DIR}" DESTINATION .)
