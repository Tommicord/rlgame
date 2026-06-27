cmake_minimum_required(VERSION 3.28)

project(rl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_MODULE_STD ON)

# Platform-specific defines for Vulkan
if (WIN32)
  add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
  add_compile_definitions(VK_USE_PLATFORM_METAL_EXT)
elseif (UNIX)
  add_compile_definitions(VK_USE_PLATFORM_XCB_KHR)
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
  add_compile_definitions(ARM_)
  message(STATUS "Target architecture ARM; NEON SIMD enabled")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
  add_compile_definitions(X86_)
  message(STATUS "Target architecture x86/x86-64; SSE/AVX SIMD enabled")
else ()
  message(WARNING "Unknown target architecture ${CMAKE_SYSTEM_PROCESSOR}; SIMD optimizations disabled")
endif ()

find_package(Vulkan REQUIRED COMPONENTS glslc)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

function(target_add_header_units TARGET)
  set(_headers ${ARGN})
  foreach (H IN LISTS _headers)
    if (IS_ABSOLUTE "${H}")
      set(HEADER_PATH "${H}")
    else ()
      set(HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${H}")
    endif ()
    target_sources(
      ${TARGET}
      PRIVATE FILE_SET
      HEADERS FILES
      "${HEADER_PATH}"
    )
  endforeach ()
endfunction()

# Helper: apply SIMD/compiler flags to a target (centralized to avoid duplication)
function(target_configure TARGET)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${TARGET} PRIVATE -mfpu=neon -mfloat-abi=hard)
    endif ()
  elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${TARGET} PRIVATE -msse4.2 -mavx2)
    elseif (MSVC)
      target_compile_options(
        ${TARGET}
        PRIVATE
        "/experimental:module"
        "/Zc:preprocessor"
        "/arch:AVX2"
        "/Zm200"
        "/MP2"
      )
    endif ()
  endif ()
endfunction()

# C++20 Modules
set(CXX_MODULES
  rl/Base/Binding.ixx
  rl/Base/Game.ixx
  rl/Base/Shader.ixx
  rl/Base/InputReceiver.ixx
  rl/Base/SimplexNoise.ixx
  rl/Base/Texture2.ixx
  rl/Base/IStateModel.ixx
  rl/Base/IStateDrawable.ixx
  rl/Base/IUpdatable.ixx
  rl/Client/State/CameraState.ixx
  rl/Client/State/UnitState.ixx
  rl/World/Camera.ixx
  rl/World/Unit/Unit.ixx
  rl/World/Unit/UnitRegistry.ixx
  rl/World/Unit/UnitResourceName.ixx
  rl/World/Unit/UnitGrass.ixx
  rl/World/Unit/UnitAir.ixx
  rl/World/Unit/UnitDynamicTexture.ixx
  rl/World/Unit/UnitPropertyStrategy.ixx
  rl/World/Unit/UnitRegister.ixx
  rl/Client/Render/Unit/UnitRendererAOTextures.ixx
  rl/Client/Render/Unit/UnitRendererBarriers.ixx
  rl/Client/Render/Unit/UnitRendererBasicBuffer.ixx
  rl/Client/Render/Unit/UnitRendererCleanup.ixx
  rl/Client/Render/Unit/UnitRendererCommandBuffers.ixx
  rl/Client/Render/Unit/UnitRendererComputePipeline.ixx
  rl/Client/Render/Unit/UnitRendererCurvatureCompute.ixx
  rl/Client/Render/Unit/UnitRendererDescriptorSets.ixx
  rl/Client/Render/Unit/UnitRendererDraw.ixx
  rl/Client/Render/Unit/UnitRendererDrawCompute.ixx
  rl/Client/Render/Unit/UnitRendererFrustum.ixx
  rl/Client/Render/Unit/UnitRendererGraphicsPipeline.ixx
  rl/Client/Render/Unit/UnitRendererShadowPipeline.ixx
  rl/Client/Render/Unit/UnitRendererIndirectDraw.ixx
  rl/Client/Render/Unit/UnitRendererInfo.ixx
  rl/Client/Render/Unit/UnitRendererNormalTextures.ixx
  rl/Client/Render/Unit/UnitRendererPlaceholderResource.ixx
  rl/Client/Render/Unit/UnitRendererShadowMap.ixx
  rl/Client/Render/Unit/UnitRendererSampler.ixx
  rl/Client/Render/Unit/UnitRendererTextureManage.ixx
  rl/Client/Render/Unit/UnitRendererVertexInput.ixx
  rl/Client/Render/Unit/UnitRendererVertices.ixx
  rl/World/Chunk/UnitChunkBuffer.ixx
  rl/World/Chunk/UnitChunkBufferGPUSimplex.ixx
)

set(SOURCES
  rl/Base/Shader.cpp
  rl/Base/InputReceiver.cpp
  rl/Base/SimplexNoise.cpp
  rl/Base/Texture2.cpp
  rl/Base/Texture2Lightning.cpp
  rl/Base/Texture2Normal.cpp
  rl/Base/Texture2Binding.cpp
  rl/Client/Render/CameraRenderer.cpp
  rl/Client/State/CameraState.cpp
  rl/Client/State/UnitState.cpp
  rl/World/Camera.cpp
  rl/World/Unit/Unit.cpp
  rl/World/Unit/UnitGrass.cpp
  rl/World/Unit/UnitGrass.cpp
  rl/World/Unit/UnitDynamicTexture.cpp
  rl/Client/Render/Unit/UnitRenderer.cpp
  rl/Client/Render/Unit/UnitRendererAOTextures.cpp
  rl/Client/Render/Unit/UnitRendererBarriers.cpp
  rl/Client/Render/Unit/UnitRendererBuffer.cpp
  rl/Client/Render/Unit/UnitRendererCleanup.cpp
  rl/Client/Render/Unit/UnitRendererCommandBuffers.cpp
  rl/Client/Render/Unit/UnitRendererComputeBuffer.cpp
  rl/Client/Render/Unit/UnitRendererComputePipeline.cpp
  rl/Client/Render/Unit/UnitRendererCurvatureCompute.cpp
  rl/Client/Render/Unit/UnitRendererDescriptorSets.cpp
  rl/Client/Render/Unit/UnitRendererDraw.cpp
  rl/Client/Render/Unit/UnitRendererDrawCompute.cpp
  rl/Client/Render/Unit/UnitRendererFrustum.cpp
  rl/Client/Render/Unit/UnitRendererGraphicsPipeline.cpp
  rl/Client/Render/Unit/UnitRendererShadowPipeline.cpp
  rl/Client/Render/Unit/UnitRendererIndirectDraw.cpp
  rl/Client/Render/Unit/UnitRendererNormalTextures.cpp
  rl/Client/Render/Unit/UnitRendererPlaceholderResource.cpp
  rl/Client/Render/Unit/UnitRendererShadowMap.cpp
  rl/Client/Render/Unit/UnitRendererSampler.cpp
  rl/Client/Render/Unit/UnitRendererSSBOBuffer.cpp
  rl/Client/Render/Unit/UnitRendererTextureManage.cpp
  rl/Client/Render/Unit/UnitRendererVertexBuffer.cpp
  rl/Client/Render/Unit/UnitRendererVertexInput.cpp
  rl/Client/Render/Unit/UnitRendererVertices.cpp
  rl/World/Chunk/UnitChunkBuffer.cpp
  rl/World/Chunk/UnitChunkBufferGPUSimplex.cpp
)

add_subdirectory(Deps/glfw)
add_subdirectory(Deps/glm)
add_subdirectory(Deps/gtest)

add_executable(rlgame rl/Base/Game.cpp ${SOURCES}
  rl/Base/Binding.ixx
  rl/World/Unit/UnitAir.ixx
  rl/World/Unit/UnitAir.cpp)

target_sources(
  rlgame
  PUBLIC
  FILE_SET CXX_MODULES
  FILES ${CXX_MODULES}
)
target_include_directories(
  rlgame
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Deps/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/Deps/stb
)
target_link_libraries(
  rlgame
  PRIVATE
  Vulkan::Vulkan
  glfw
  glm::glm
)

# Platform-specific libraries for input polling
if (WIN32)
  # Windows API is part of the OS, no additional libraries needed
elseif (UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
  target_link_libraries(rlgame PRIVATE X11::X11)
elseif (APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)
  target_link_libraries(rlgame PRIVATE ${COCOA_LIBRARY} ${APPLICATIONSERVICES_LIBRARY})
elseif (ANDROID)
  target_link_libraries(rlgame PRIVATE android log)
elseif (IOS)
  target_link_libraries(rlgame PRIVATE "-framework UIKit" "-framework Foundation")
endif ()
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
  rl/Shaders/unit.shadow.vert
  rl/Shaders/unit.shadow.frag
  rl/Shaders/unit.face.comp
  rl/Shaders/unit.tessel.comp
  rl/Shaders/unit.frag
  rl/Shaders/unit.vert
  rl/Shaders/Simplex/simplex.comp
  rl/Shaders/Simplex/simplex.init.comp
)
enable_testing()

add_executable(
  rlgame_tests
  ${SOURCES}
  Tests/UnitDynamicTextureTest.cpp
  Tests/UnitChunkBufferTest.cpp
)
target_sources(
  rlgame_tests
  PUBLIC FILE_SET
  CXX_MODULES FILES
  ${CXX_MODULES}
)

target_include_directories(
  rlgame_tests
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Deps/stb
)

target_compile_options(rlgame_tests PRIVATE /EHsc)
set_property(TARGET rlgame_tests PROPERTY CXX_STANDARD 20)
set_property(TARGET rlgame_tests PROPERTY CXX_STANDARD_REQUIRED ON)

set(EXTERN_IMPL 
  GLFW_INCLUDE_VULKAN 
  STB_IMAGE_IMPLEMENTATION
)

target_compile_definitions(
  rlgame 
  PRIVATE 
  ${EXTERN_IMPL}
)
target_compile_definitions(
  rlgame_tests
  PRIVATE 
  ${EXTERN_IMPL}
)
target_link_libraries(
  rlgame_tests
  PRIVATE
  Vulkan::Vulkan
  gtest_main
  glm::glm
  glfw
)

set(HEADER_UNITS
  Deps/stb/stb_image.h
  Deps/glm/glm/glm.hpp
  Deps/glfw/include/GLFW/glfw3.h)
target_configure(rlgame)
target_configure(rlgame_tests)

# Precompile headers
target_add_header_units(rlgame_tests ${HEADER_UNITS})
target_add_header_units(rlgame ${HEADER_UNITS})

# Enable SIMD instructions based on architecture for tests
if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rlgame_tests PRIVATE -mfpu=neon)
    target_compile_options(rlgame_tests PRIVATE -mfloat-abi=hard)
  endif ()
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rlgame_tests PRIVATE -msse4.2)
    target_compile_options(rlgame_tests PRIVATE -mavx2)
  elseif (MSVC)
    target_compile_options(rlgame_tests PRIVATE /arch:AVX2)
  endif ()
endif ()

include(GoogleTest)

function(target_add_resource TARGET)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs SOURCES DESTINATIONS)
  cmake_parse_arguments(
    PARSE_ARGV 0
    PREFIX
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
  )
  list(LENGTH PREFIX_SOURCES SRCS)
  list(LENGTH PREFIX_DESTINATIONS DSTS)
  if(SRCS EQUAL 0 OR DSTS EQUAL 0 OR NOT SRCS EQUAL DSTS)
    message(
      FATAL_ERROR
      "Target: ${TARGET}, "
      "required at least one source "
      "and one destination"
    )
  endif()

  message(STATUS "Copying resources to build; target: ${TARGET}")
  foreach (SRC DST IN ZIP_LISTS PREFIX_SOURCES PREFIX_DESTINATIONS)
    if(SRC STREQUAL "")
      message(FATAL_ERROR "The source cannot be empty")
    endif ()

    if(IS_ABSOLUTE "${SRC}")
      set(SRC_PATH "${SRC}")
    else()
      set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
    endif()

    if(DST STREQUAL "CURRENT_DIR")
      set(DEST_PATH "$<TARGET_FILE_DIR:${TARGET}>")
    else()
      set(DEST_PATH "$<TARGET_FILE_DIR:${TARGET}>/${DST}")
    endif()

    add_custom_command(
      TARGET ${TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${SRC_PATH}"
      "${DEST_PATH}"
      COMMENT "Copying ${SRC_PATH} to ${DEST_PATH}"
      VERBATIM
    )
  endforeach()
endfunction()

target_add_resource(rlgame
  SOURCES
    Public/Resources/Textures
    ${CMAKE_CURRENT_BINARY_DIR}/Shaders
  DESTINATIONS
    CURRENT_DIR
    Shaders/
)
target_add_resource(
  rlgame_tests
  SOURCES
    Public/Resources/Textures
  DESTINATIONS
    CURRENT_DIR
)

gtest_discover_tests(rlgame_tests)