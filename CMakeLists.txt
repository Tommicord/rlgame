cmake_minimum_required(VERSION 4.2.0)

project(rl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED COMPONENTS glslc)
include(FetchContent)
FetchContent_Declare(
  glm
  GIT_REPOSITORY	https://github.com/g-truc/glm.git
  GIT_TAG 	      0af55ccecd98d4e5a8d1fad7de25ba429d60e863 #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(SOURCES 
  rl/Base/Game.cpp
  rl/Base/Game.h
  rl/Base/ShaderFactory.cpp
  rl/Base/ShaderFactory.h
  rl/Base/DeviceInputReceiver.h
  rl/Base/DeviceInputReceiver.cpp
  rl/Base/CameraProvider.h
  rl/Base/CameraProvider.cpp
  rl/Client/Render/CameraRenderer.cpp
  rl/Base/SimplexNoise.cpp
  rl/Base/SimplexNoise.h
  rl/World/Unit/Unit.cpp
  rl/World/Unit/Unit.h
  rl/World/Unit/UnitResourceName.h
  rl/Base/Texture2.cpp
  rl/Base/Texture2.h
  rl/World/Unit/UnitGrass.cpp
  rl/World/Unit/UnitGrass.h
  rl/World/Unit/UnitDynamicTexture.cpp
  rl/World/Unit/UnitDynamicTexture.h)

add_subdirectory(Deps/glfw)
add_executable(rlgame ${SOURCES})
target_include_directories(
  rlgame
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${stb_SOURCE_DIR}
)
target_link_libraries(
  rlgame
  PRIVATE
  Vulkan::Vulkan
  glfw
  glm::glm
)

# Platform-specific libraries for input polling
if(WIN32)
  # Windows API is part of the OS, no additional libraries needed
elseif(UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
  target_link_libraries(rlgame PRIVATE X11::X11)
elseif(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)
  target_link_libraries(rlgame PRIVATE ${COCOA_LIBRARY} ${APPLICATIONSERVICES_LIBRARY})
elseif(ANDROID)
  target_link_libraries(rlgame PRIVATE android log)
elseif(IOS)
  target_link_libraries(rlgame PRIVATE "-framework UIKit" "-framework Foundation")
endif()
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
enable_testing()

add_executable(
  rlgame_tests 
  Tests/UnitDynamicTextureTest.cpp
  Tests/Main.cpp
  rl/World/Unit/UnitDynamicTexture.cpp
  rl/Base/Texture2.cpp
  rl/Base/SimplexNoise.cpp
  rl/World/Unit/UnitResourceName.h
)

target_include_directories(
  rlgame_tests
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${stb_SOURCE_DIR}
)

target_compile_options(rlgame_tests PRIVATE /EHsc)
set_property(TARGET rlgame_tests PROPERTY CXX_STANDARD 20)
set_property(TARGET rlgame_tests PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(
  rlgame_tests
  PRIVATE
  Vulkan::Vulkan
  gtest_main
  glm::glm
  glfw
)

include(GoogleTest)

add_custom_command(
  TARGET rlgame_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/Public/Resources/Textures/"
  "$<TARGET_FILE_DIR:rlgame_tests>/"
  COMMENT "Copying test assets to the build directory"
)
gtest_discover_tests(rlgame_tests)