cmake_minimum_required(VERSION 4.2.0)

project(rl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED COMPONENTS glslc)
add_subdirectory(Deps/glfw)
add_executable(
  rlgame
  rl/Base/Game.cpp
  rl/Base/Game.h
  rl/Base/ShaderFactory.cpp
  rl/Base/ShaderFactory.h
  rl/Base/DeviceInputReceiver.h
  rl/Base/DeviceInputReceiver.cpp
  rl/Base/CameraProvider.h
  rl/Base/CameraProvider.cpp
)
target_include_directories(
  rlgame
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
  rlgame
  PRIVATE
  Vulkan::Vulkan
  glfw
)
# Compile Vulkan Shaders
function(target_compile_shaders TARGET)
  set(SHADERS ${ARGN})
  set(SPV_OUTPUTS "")

  foreach (SHADER IN LISTS SHADERS)
    get_filename_component(
      SHADER_ABS_PATH
      "${SHADER}"
      ABSOLUTE)
    get_filename_component(
      SHADER_NAME
      "${SHADER}"
      NAME)
    set(SPV "${CMAKE_CURRENT_BINARY_DIR}/Shaders/${SHADER_NAME}.spv")
    add_custom_command(
      OUTPUT "${SPV}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/Shaders/"
      COMMAND Vulkan::glslc "${SHADER_ABS_PATH}" -o "${SPV}"
      DEPENDS "${SHADER_ABS_PATH}"
      COMMENT "Compiling shader ${SHADER_NAME}"
      VERBATIM
    )
    list(APPEND SPV_OUTPUTS "${SPV}")
  endforeach ()
  target_sources(${TARGET} PRIVATE ${SPV_OUTPUTS})
endfunction()

target_compile_shaders(
  rlgame
  Public/Shaders/shader.frag
  Public/Shaders/shader.vert
)