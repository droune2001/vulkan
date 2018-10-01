# TODO: find the var for "current sub directory"
set(CURRENT_TARGET particles_loop)

set(SHADER_SOURCE_DIR "${ASSETS_DIR}/${CURRENT_TARGET}")
set(CURRENT_SHADER_PROJECT "${CURRENT_TARGET}_shaders")

set(GLSL_VALIDATOR "$ENV{VULKAN_SDK_DIR}/Bin/glslangValidator.exe")

file( GLOB CURRENT_TARGET_SOURCES "*.c*" )
file( GLOB CURRENT_TARGET_HEADERS "*.h*" )
file( GLOB CURRENT_TARGET_SHADERS 
   "${SHADER_SOURCE_DIR}/*.frag"
   "${SHADER_SOURCE_DIR}/*.vert"
   "${SHADER_SOURCE_DIR}/*.comp")
   
source_group( "Sources" FILES ${CURRENT_TARGET_SOURCES} )
source_group( "Headers" FILES ${CURRENT_TARGET_HEADERS} )
source_group( "Shaders" FILES ${CURRENT_TARGET_SHADERS} )

add_executable(${CURRENT_TARGET}
    ${CURRENT_TARGET_SOURCES} 
    ${CURRENT_TARGET_HEADERS} 
    ${CURRENT_TARGET_SHADERS})

# does not seem to work. You have to type by hand the same exact string $(TargetDir), anf then it works...
#set_target_properties(${CURRENT_TARGET} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir)") # using Visual Studio macro
set_target_properties(${CURRENT_TARGET} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)") # using Visual Studio macro

foreach(GLSL ${CURRENT_TARGET_SHADERS})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/data/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/data/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
	COMMENT "Compiling shader ${GLSL}")
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    ${CURRENT_SHADER_PROJECT} 
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(${CURRENT_TARGET} ${CURRENT_SHADER_PROJECT})

add_custom_command(
    TARGET ${CURRENT_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${CURRENT_TARGET}>/data/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/data"
        "$<TARGET_FILE_DIR:${CURRENT_TARGET}>/data"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSETS_DIR}/imgui.ini" "$<TARGET_FILE_DIR:${CURRENT_TARGET}>/imgui.ini"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSETS_DIR}/vk_layer_settings.txt" "$<TARGET_FILE_DIR:${CURRENT_TARGET}>/vk_layer_settings.txt"
)

install(TARGETS ${CURRENT_TARGET} DESTINATION .)
#install(DIRECTORY "${SHADER_SOURCE_DIR}" DESTINATION .)

# set( CMAKE_INCLUDE_CURRENT_DIR ON )

# target_link_libraries(particles_loop vulkan_lib)

# set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:atlthunk.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:msvcrt.lib /SAFESEH:NO")
# set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:atlthunk.lib /NODEFAULTLIB:libcmt.lib /SAFESEH:NO")


# ${PROJECT_BINARY_DIR} => /build
# ${CMAKE_BINARY_DIR} => /build
# ${CMAKE_CURRENT_BINARY_DIR} => /build/particles_loop
# ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} => /build/bin
# $<TARGET_FILE_DIR:${CURRENT_TARGET}> => /build/bin/<CONFIG>
