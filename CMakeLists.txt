cmake_minimum_required(VERSION 3.28)

project(rl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

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

function(target_configure TARGET)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${TARGET} PRIVATE -mfpu=neon -mfloat-abi=hard)
    endif ()
  elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${TARGET} PRIVATE -msse4.2 -mavx2 -std=c++20 -fmodules-ts)
    elseif (MSVC)
      target_compile_options(
        ${TARGET}
        PRIVATE
        "/Zc:__cplusplus"
        "/Zc:preprocessor"
        "/arch:AVX2"
        "/MP" "/Zm200" "/FS"
        "/std:c++20"
      )
    endif ()
  endif ()
endfunction()

set(CXX_MODULES
  rl/Base/Game.ixx
  rl/Base/Binding.ixx
  rl/Base/Game.ixx
  rl/Base/Shader.ixx
  rl/Base/UserInput.ixx
  rl/Base/SimplexNoise.ixx
  rl/Base/Texture2.ixx
  rl/Base/IStateModel.ixx
  rl/Base/IStateDrawable.ixx
  rl/Base/IUpdatable.ixx
  rl/Player/PlayerCamera.ixx
  rl/Player/PlayerCameraController.ixx
  rl/Player/PlayerController.ixx
  rl/Player/PlayerProvider.ixx
  rl/Player/IPlayer.ixx
  rl/Player/IPlayerController.ixx
  rl/Player/IPlayerCameraController.ixx
  rl/Player/Player.ixx
  rl/World/Unit/Unit.ixx
  rl/World/Unit/UnitRegistry.ixx
  rl/World/Unit/UnitResourceName.ixx
  rl/World/Unit/UnitGrass.ixx
  rl/World/Unit/UnitMantle.ixx
  rl/World/Unit/UnitAir.ixx
  rl/World/Unit/UnitDynamicTexture.ixx
  rl/World/Unit/UnitPropertyStrategy.ixx
  rl/World/Unit/UnitRegister.ixx
  rl/World/Unit/UnitGPUParams.ixx
  rl/World/Unit/UnitRegistryGPU.ixx
  rl/World/Unit/UnitGPURegisterDerived.ixx
  rl/World/Unit/UnitGrassGrowBehavior.ixx
  rl/World/Unit/UnitTextureFactory.ixx
  rl/Client/State/UnitState.ixx
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
  rl/Client/Render/Unit/UnitRendererMeshGen.ixx
  rl/World/Chunk/UnitChunkBuffer.ixx
  rl/World/Chunk/UnitGPUSimplexNoise.ixx
  rl/World/Chunk/ChunkGPUNoiseGen.ixx
  rl/World/Chunk/UnitChunkBufferFBM.ixx
  rl/World/Chunk/ChunkErrorRecovery.ixx
  rl/World/Chunk/ChunkInRenderUnits.ixx
  rl/World/Chunk/ChunkThreadPool.ixx
  rl/World/Chunk/ChunkSystem.ixx
  rl/World/Chunk/ChunkLogger.ixx
  rl/World/Chunk/ChunkTransaction.ixx
  rl/World/Chunk/ChunkTransactionRingBuffer.ixx
  rl/World/Chunk/UnitChunkAccessor.ixx
  rl/World/Chunk/ChunkGeneratorGPU.ixx
  rl/World/Time/TimeSystem.ixx
  rl/World/Skybox/SkyboxSystem.ixx
  rl/World/ServiceLocator.ixx
  rl/World/ServiceUpdater.ixx
  rl/World/ServiceUpdaterRegistry.ixx
  rl/World/ServiceUpdaterRegister.ixx
  rl/World/Biome/BaseBiome.ixx
  rl/World/Biome/BiomeGPUParams.ixx
  rl/World/Biome/BiomeRegistryGPU.ixx
  rl/World/Biome/IBiome.ixx
  rl/RayLog/RayLogConfig.ixx
  rl/RayLog/RayLogPlatform.ixx
  rl/RayLog/RayLogUuid.ixx
  rl/RayLog/RayLogLevelPrinter.ixx
  rl/RayLog/IRayLogSerializable.ixx
  rl/RayLog/RayLogRingBuffer.ixx
  rl/RayLog/RayLogMessage.ixx
  rl/RayLog/RayLogColorPrinter.ixx
  rl/RayLog/RayLogTimestamp.ixx
  rl/RayLog/RayLogMessageBuilder.ixx
  rl/RayLog/RayLogPlatformOutput.ixx
  rl/RayLog/RayLogFormatter.ixx
  rl/RayLog/RayLogFormatParser.ixx
  rl/RayLog/RayLogThreadPool.ixx
  rl/RayLog/RayLogFileOutput.ixx
  rl/RayLog/RayLogStackTrace.ixx
  rl/RayLog/RayLogSymbolDemangler.ixx
  rl/RayLog/RayLogFatalHandler.ixx
  rl/RayLog/RayLog.ixx
  rl/RayLog/RayLogMacro.ixx
  rl/World/Chunk/ChunkSystemNotify.ixx
)

set(SOURCES
  rl/Base/Game.cpp
  rl/Base/Shader.cpp
  rl/Base/UserInput.cpp
  rl/Base/SimplexNoise.cpp
  rl/Base/Texture2.cpp
  rl/Base/Texture2Lightning.cpp
  rl/Base/Texture2Normal.cpp
  rl/Base/Texture2Binding.cpp
  rl/Player/PlayerCameraController.cpp
  rl/Player/PlayerController.cpp
  rl/Player/PlayerCamera.cpp
  rl/Player/Player.cpp
  rl/World/Unit/Unit.cpp
  rl/World/Unit/UnitDynamicTexture.cpp
  rl/World/Unit/UnitTextureFactory.cpp
  rl/World/Unit/UnitRegistryGPU.cpp
  rl/World/Unit/UnitGrass.cpp
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
  rl/Client/Render/Unit/UnitRendererMeshGen.cpp
  rl/World/Biome/BiomeRegistryGPU.cpp
  rl/World/Chunk/UnitChunkBuffer.cpp
  rl/World/Chunk/UnitGPUSimplexNoise.cpp
  rl/World/Chunk/ChunkInRenderUnits.cpp
  rl/World/Chunk/ChunkSystem.cpp
  rl/World/Chunk/ChunkErrorRecovery.cpp
  rl/World/Chunk/ChunkLogger.cpp
  rl/World/Chunk/ChunkThreadPool.cpp
  rl/World/Chunk/ChunkTransactionRingBuffer.cpp
  rl/World/Chunk/UnitChunkAccessor.cpp
  rl/World/Chunk/ChunkGeneratorGPU.cpp
  rl/World/Time/TimeSystem.cpp
  rl/World/Skybox/SkyboxSystem.cpp
  rl/World/ServiceLocator.cpp
  rl/World/ServiceUpdater.cpp
  rl/World/ServiceUpdaterRegistry.cpp
  rl/World/ServiceUpdaterRegister.cpp
  rl/World/Unit/UnitGrassGrowBehavior.cpp
  rl/World/Chunk/ChunkSystemNotify.cpp
)

add_subdirectory(Deps/glfw)
add_subdirectory(Deps/glm)
add_subdirectory(Deps/gtest)
include(GoogleTest)

add_executable(rlgame rl/Base/Main.cpp ${SOURCES})

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
  target_link_libraries(rlgame PRIVATE dbghelp)
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
  rl/Shaders/unit.mesh.comp
  rl/Shaders/unit.frag
  rl/Shaders/unit.vert
  rl/Shaders/World/world.biome.comp
  rl/Shaders/World/world.gen.pol.comp
  rl/Shaders/World/world.heightmap.comp
  rl/Shaders/World/world.noise.comp
  rl/Shaders/World/world.unitplace.comp
  rl/Shaders/Simplex/simplex.comp
  rl/Shaders/Simplex/simplex.init.comp
)
enable_testing()

add_executable(
  rlgame_tests
  ${SOURCES}
  Tests/UnitDynamicTextureTest.cpp
  Tests/UnitChunkBufferTest.cpp
  Tests/RayLogRingBufferTest.cpp
  Tests/RayLogUuidTest.cpp
  Tests/RayLogFormatterTest.cpp
  Tests/RayLogFormatParserTest.cpp
  Tests/RayLogTimestampTest.cpp
  Tests/RayLogMessageTest.cpp
  Tests/RayLogLevelPrinterTest.cpp
  Tests/RayLogConfigTest.cpp
  Tests/RayLogPlatformTest.cpp
  Tests/ChunkErrorRecoveryTest.cpp
  Tests/ChunkInRenderUnitsTest.cpp
  Tests/ChunkLoggerTest.cpp
  Tests/ChunkSystemTest.cpp
  Tests/ChunkThreadPoolTest.cpp
  Tests/ChunkTransactionRingBufferTest.cpp
  Tests/ChunkTransactionTest.cpp
  Tests/UnitChunkAccessorTest.cpp
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
if (WIN32)
  target_link_libraries(rlgame_tests PRIVATE dbghelp)
endif ()

target_configure(rlgame)
target_configure(rlgame_tests)

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
  if (SRCS EQUAL 0 OR DSTS EQUAL 0 OR NOT SRCS EQUAL DSTS)
    message(
      FATAL_ERROR
      "Target: ${TARGET}, "
      "required at least one source "
      "and one destination"
    )
  endif ()

  message(STATUS "Copying resources to build; target: ${TARGET}")
  foreach (SRC DST IN ZIP_LISTS PREFIX_SOURCES PREFIX_DESTINATIONS)
    if (SRC STREQUAL "")
      message(FATAL_ERROR "The source cannot be empty")
    endif ()

    if (IS_ABSOLUTE "${SRC}")
      set(SRC_PATH "${SRC}")
    else ()
      set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
    endif ()

    if (DST STREQUAL "CURRENT_DIR")
      set(DEST_PATH "$<TARGET_FILE_DIR:${TARGET}>")
    else ()
      set(DEST_PATH "$<TARGET_FILE_DIR:${TARGET}>/${DST}")
    endif ()

    add_custom_command(
      TARGET ${TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${SRC_PATH}"
      "${DEST_PATH}"
      COMMENT "Copying ${SRC_PATH} to ${DEST_PATH}"
      VERBATIM
    )
  endforeach ()
endfunction()

target_add_resource(rlgame
  SOURCES
  Public/Textures
  ${CMAKE_CURRENT_BINARY_DIR}/Shaders
  DESTINATIONS
  CURRENT_DIR
  Shaders/
)
target_add_resource(
  rlgame_tests
  SOURCES
  Public/Textures
  DESTINATIONS
  CURRENT_DIR
)

gtest_discover_tests(rlgame_tests)