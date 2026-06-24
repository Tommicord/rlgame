cmake_minimum_required(VERSION 4.2.0)

project(rl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect target architecture and set appropriate SIMD flags
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
  add_compile_definitions(ARM_)
  message(STATUS "Target architecture ARM - NEON SIMD enabled")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
  add_compile_definitions(X86_)
  message(STATUS "Target architecture x86/x86-64 - SSE/AVX SIMD enabled")
else()
  message(WARNING "Unknown target architecture ${CMAKE_SYSTEM_PROCESSOR} - SIMD optimizations disabled")
endif()

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
  rl/Base/InputReceiver.h
  rl/Base/InputReceiver.cpp
  rl/Base/SimplexNoise.cpp
  rl/Base/SimplexNoise.h
  rl/Base/Texture2.cpp
  rl/Base/Texture2Lightning.cpp
  rl/Base/Texture2Normal.cpp
  rl/Base/Texture2Vk.cpp
  rl/Base/Texture2.h
  rl/Base/StateModel.h
  rl/Base/IUpdatable.h
  rl/Client/Render/CameraRenderer.cpp
  rl/Client/State/CameraState.cpp
  rl/Client/State/CameraState.h
  rl/Client/State/UnitState.cpp
  rl/Client/State/UnitState.h
  rl/World/Camera.h
  rl/World/Camera.cpp
  rl/World/Unit/Unit.cpp
  rl/World/Unit/Unit.h
  rl/World/Unit/UnitRegistry.h
  rl/World/Unit/UnitResourceName.h
  rl/World/Unit/UnitGrass.cpp
  rl/World/Unit/UnitGrass.h
  rl/World/Unit/UnitDynamicTexture.cpp
  rl/World/Unit/UnitDynamicTexture.h
  rl/World/Unit/UnitRegistry.h
  rl/World/Unit/UnitRegister.h
  rl/Client/Render/Unit/UnitRenderer.cpp
  rl/Client/Render/Unit/UnitRendererAOTextures.cpp
  rl/Client/Render/Unit/UnitRendererAOTextures.h
  rl/Client/Render/Unit/UnitRendererBarriers.cpp
  rl/Client/Render/Unit/UnitRendererBarriers.h
  rl/Client/Render/Unit/UnitRendererBasicBuffer.h
  rl/Client/Render/Unit/UnitRendererBuffer.cpp
  rl/Client/Render/Unit/UnitRendererCleanup.cpp
  rl/Client/Render/Unit/UnitRendererCleanup.h
  rl/Client/Render/Unit/UnitRendererCommandBuffers.h
  rl/Client/Render/Unit/UnitRendererCommandBuffers.cpp
  rl/Client/Render/Unit/UnitRendererComputeBuffer.cpp
  rl/Client/Render/Unit/UnitRendererComputePipeline.cpp
  rl/Client/Render/Unit/UnitRendererComputePipeline.h
  rl/Client/Render/Unit/UnitRendererCurvatureCompute.cpp
  rl/Client/Render/Unit/UnitRendererCurvatureCompute.h
  rl/Client/Render/Unit/UnitRendererDescriptorSets.cpp
  rl/Client/Render/Unit/UnitRendererDescriptorSets.h
  rl/Client/Render/Unit/UnitRendererDraw.cpp
  rl/Client/Render/Unit/UnitRendererDraw.h
  rl/Client/Render/Unit/UnitRendererDrawCompute.cpp
  rl/Client/Render/Unit/UnitRendererDrawCompute.h
  rl/Client/Render/Unit/UnitRendererFrustum.cpp
  rl/Client/Render/Unit/UnitRendererFrustum.h
  rl/Client/Render/Unit/UnitRendererGraphicsPipeline.cpp
  rl/Client/Render/Unit/UnitRendererGraphicsPipeline.h
  rl/Client/Render/Unit/UnitRendererIndirectDraw.cpp
  rl/Client/Render/Unit/UnitRendererIndirectDraw.h
  rl/Client/Render/Unit/UnitRendererInfo.h
  rl/Client/Render/Unit/UnitRendererLightingBlock.h
  rl/Client/Render/Unit/UnitRendererNormalTextures.cpp
  rl/Client/Render/Unit/UnitRendererNormalTextures.h
  rl/Client/Render/Unit/UnitRendererPlaceholderResource.cpp
  rl/Client/Render/Unit/UnitRendererPlaceholderResource.h
  rl/Client/Render/Unit/UnitRendererSampler.cpp
  rl/Client/Render/Unit/UnitRendererSampler.h
  rl/Client/Render/Unit/UnitRendererSSBOBuffer.cpp
  rl/Client/Render/Unit/UnitRendererTextureManage.cpp
  rl/Client/Render/Unit/UnitRendererTextureManage.h
  rl/Client/Render/Unit/UnitRendererVertexBuffer.cpp
  rl/Client/Render/Unit/UnitRendererVertexInput.cpp
  rl/Client/Render/Unit/UnitRendererVertexInput.h
  rl/Client/Render/Unit/UnitRendererVertices.cpp
  rl/Client/Render/Unit/UnitRendererVertices.h
)

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

# Enable SIMD instructions based on architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rlgame PRIVATE -mfpu=neon)
    target_compile_options(rlgame PRIVATE -mfloat-abi=hard)
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rlgame PRIVATE -msse4.2)
    target_compile_options(rlgame PRIVATE -mavx2)
  elseif(MSVC)
    target_compile_options(rlgame PRIVATE /arch:AVX2)
  endif()
endif()

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
  Public/Shaders/unit.face.comp
  Public/Shaders/unit.curve.comp
  Public/Shaders/unit.lightning.frag
  Public/Shaders/unit.vert
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

# Enable SIMD instructions based on architecture for tests
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rlgame_tests PRIVATE -mfpu=neon)
    target_compile_options(rlgame_tests PRIVATE -mfloat-abi=hard)
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rlgame_tests PRIVATE -msse4.2)
    target_compile_options(rlgame_tests PRIVATE -mavx2)
  elseif(MSVC)
    target_compile_options(rlgame_tests PRIVATE /arch:AVX2)
  endif()
endif()

include(GoogleTest)

function(target_add_resources TARGET)
  add_custom_command(
    TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Public/Resources/Textures/"
    "$<TARGET_FILE_DIR:rlgame_tests>/"
    COMMENT "Copying test assets to the build directory"
  )
endfunction()

target_add_resources(rlgame)
target_add_resources(rlgame_tests)

gtest_discover_tests(rlgame_tests)