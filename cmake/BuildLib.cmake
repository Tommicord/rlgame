file(GLOB_RECURSE CSTL_SOURCES          CONFIGURE_DEPENDS src/rlgame.base/cstl/*.c)
file(GLOB_RECURSE CSTL_HEADERS          CONFIGURE_DEPENDS src/rlgame.base/cstl/*.h)
file(GLOB_RECURSE CVULKAN_SOURCES       CONFIGURE_DEPENDS src/rlgame.base/cvulkan/*.c)
file(GLOB_RECURSE CVULKAN_HEADERS       CONFIGURE_DEPENDS src/rlgame.base/cvulkan/*.h)
file(GLOB_RECURSE CVULKAN_CUDA_SOURCES  CONFIGURE_DEPENDS src/rlgame.base/cvulkan/*.cu)
if(NOT CUDA_FOUND)
  set(CVULKAN_CUDA_SOURCES "")
endif()
file(GLOB_RECURSE CVULKAN_CPU_SOURCES   CONFIGURE_DEPENDS src/rlgame.base/cvulkan_cpu/*.c)
file(GLOB_RECURSE CVULKAN_CPU_HEADERS   CONFIGURE_DEPENDS src/rlgame.base/cvulkan_cpu/*.h)
file(GLOB_RECURSE GAME_SOURCES          CONFIGURE_DEPENDS src/rlgame.base/game/*.c)
file(GLOB_RECURSE GAME_HEADERS          CONFIGURE_DEPENDS src/rlgame.base/game/*.h)
file(GLOB         CLIENT_RENDER_SOURCES CONFIGURE_DEPENDS src/rlgame.client/render/*.c)
file(GLOB         CLIENT_RENDER_HEADERS CONFIGURE_DEPENDS src/rlgame.client/render/*.h)
file(GLOB         MAIN_SOURCES          CONFIGURE_DEPENDS src/rlgame.base/*.c)
file(GLOB         MAIN_HEADERS          CONFIGURE_DEPENDS src/rlgame.base/*.h)
file(GLOB         RPACK_SOURCES         CONFIGURE_DEPENDS consoletools/rpack/*.c)
file(GLOB         RPACK_HEADERS         CONFIGURE_DEPENDS consoletools/rpack/*.h)
file(GLOB         RPACK_CUDA_SOURCES    CONFIGURE_DEPENDS src/rlgame.compsrc/rpack_mipmap.cu)
file(GLOB_RECURSE MICROBIT_SOURCES      CONFIGURE_DEPENDS consoletools/microbit/*.c)
file(GLOB_RECURSE MICROBIT_HEADERS      CONFIGURE_DEPENDS consoletools/microbit/*.h)
if(NOT CUDA_FOUND)
  set(RPACK_CUDA_SOURCES)
endif()
list(FILTER MAIN_SOURCES EXCLUDE REGEX "main\\.(c|h)$")
list(FILTER RPACK_SOURCES EXCLUDE REGEX "rpack_main\\.c$")

# GPU backend detection: Try CUDA first, fallback to OpenCL
set(GPU_BACKEND_MACRO "")

# Try CUDA first
find_package(CUDA QUIET)
if(CUDA_FOUND)
  enable_language(CUDA)
  set(GPU_BACKEND_MACRO "R_CUDA")
  message(STATUS "GPU backend: CUDA detected and enabled")
else()
  # Fallback to OpenCL
  find_package(OpenCL QUIET)
  if(OpenCL_FOUND)
    set(GPU_BACKEND_MACRO "R_OPENCL")
    message(STATUS "GPU backend: OpenCL detected and enabled (CUDA not available)")
  else()
    message(STATUS "GPU backend: Neither CUDA nor OpenCL detected, using CPU fallback")
  endif()
endif()

function(apply_gpu_backend TARGET)
  if(GPU_BACKEND_MACRO)
    target_compile_definitions(${TARGET} PRIVATE ${GPU_BACKEND_MACRO})
  endif()
endfunction()

add_library(rlgame.base.cstl SHARED ${CSTL_SOURCES} ${CSTL_HEADERS})

if (WIN32)
  target_link_libraries(rlgame.base.cstl PUBLIC dbghelp)
endif()

target_compile_definitions(rlgame.base.cstl PRIVATE $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG> R_CSTL_BUILDING_DLL)
set_common_output_directories(rlgame.base.cstl)
set_base_include_directories(rlgame.base.cstl)
apply_gpu_backend(rlgame.base.cstl)
add_library(rlgame.base.cvulkan SHARED ${CVULKAN_SOURCES} ${CVULKAN_HEADERS} ${CVULKAN_CUDA_SOURCES})

set_common_output_directories(rlgame.base.cvulkan)
set_base_include_directories(rlgame.base.cvulkan)
target_compile_definitions(rlgame.base.cvulkan PUBLIC $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG> R_CVULKAN_BUILDING_DLL)
apply_gpu_backend(rlgame.base.cvulkan)

target_link_libraries(
  rlgame.base.cvulkan
  PUBLIC
  Vulkan::Vulkan
  rlgame.base.cstl
)

if(OpenCL_FOUND)
  target_link_libraries(rlgame.base.cvulkan PUBLIC OpenCL::OpenCL)
endif()

if(CUDA_FOUND)
  target_link_libraries(rlgame.base.cvulkan PUBLIC CUDA::CUDA)
endif()

add_library(rlgame.base.game SHARED ${GAME_SOURCES} ${GAME_HEADERS})
set_common_output_directories(rlgame.base.game)
set_base_include_directories(rlgame.base.game)
target_compile_definitions(rlgame.base.game PUBLIC $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG> R_GAME_BUILDING_DLL)
apply_gpu_backend(rlgame.base.game)

target_link_libraries(
  rlgame.base.game
  PUBLIC
  rlgame.base.cvulkan
  rlgame.base.cstl
)

add_library(rlgame.base.rpack SHARED ${RPACK_SOURCES} ${RPACK_HEADERS} ${RPACK_CUDA_SOURCES})
set_common_output_directories(rlgame.base.rpack)
set_base_include_directories(rlgame.base.rpack)
target_compile_definitions(rlgame.base.rpack PUBLIC $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG> R_PACK_BUILDING_DLL)
apply_gpu_backend(rlgame.base.rpack)

target_link_libraries(
  rlgame.base.rpack
  PUBLIC
  rlgame.base.cstl
)
if (UNIX AND NOT APPLE)
  target_link_libraries(rlgame.base.rpack PUBLIC m)
endif()
if(OpenCL_FOUND)
  target_link_libraries(rlgame.base.rpack PUBLIC OpenCL::OpenCL)
endif()
if(CUDA_FOUND)
  target_link_libraries(rlgame.base.rpack PUBLIC CUDA::CUDA)
endif()

add_library(rlgame.client.render SHARED ${CLIENT_RENDER_SOURCES} ${CLIENT_RENDER_HEADERS})
set_common_output_directories(rlgame.client.render)
set_base_include_directories(rlgame.client.render)
target_compile_definitions(rlgame.client.render PUBLIC $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG> R_RENDER_BUILDING_DLL)
apply_gpu_backend(rlgame.client.render)

target_link_libraries(
  rlgame.client.render
  PUBLIC
  rlgame.base.game
  rlgame.base.cvulkan
  rlgame.base.cstl
)

add_library(
  rlgame.base.entry SHARED ${MAIN_SOURCES} ${MAIN_HEADERS}
)
set_common_output_directories(rlgame.base.entry)
set_base_include_directories(rlgame.base.entry)
target_compile_definitions(rlgame.base.entry PUBLIC R_ENTRY_BUILDING_DLL)
target_link_libraries(rlgame.base.entry PUBLIC rlgame.base.cstl)
link_wayland(rlgame.base.entry)
link_xcb(rlgame.base.entry)
apply_gpu_backend(rlgame.base.entry)

if (APPLE)
  find_package(MoltenVK REQUIRED)
  if (MoltenVK_FOUND)
    target_link_libraries(rlgame.base.entry PUBLIC MoltenVK::MoltenVK)
    target_compile_definitions(rlgame.base.entry PUBLIC VK_USE_PLATFORM_METAL_EXT)
  endif()
endif()

option(RL_BUILD "Build the rlgame executable" ON)
if(RL_BUILD)
  set(MAIN_SOURCE src/rlgame.base/main.c)
  if(NOT WIN32)
    add_executable(rlgame ${MAIN_SOURCE} ${MAIN_SOURCES} ${MAIN_HEADERS})
  else()
    add_executable(rlgame WIN32 ${MAIN_SOURCE})
  endif()
  set_common_output_directories(rlgame)
  link_base_libraries(rlgame)
  link_wayland(rlgame)
  link_xcb(rlgame)
  link_x11(rlgame)
endif()

add_executable(rlgame_rpack consoletools/rpack/rpack_main.c)
target_include_directories(rlgame_rpack PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_common_output_directories(rlgame_rpack)
target_link_libraries(rlgame_rpack PRIVATE rlgame.base.rpack)
apply_gpu_backend(rlgame_rpack)

if (RL_BUILD AND APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)
  find_library(FOUNDATION_LIBRARY Foundation)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  target_link_libraries(rlgame PRIVATE ${COCOA_LIBRARY} ${APPLICATIONSERVICES_LIBRARY} ${FOUNDATION_LIBRARY} ${COREFOUNDATION_LIBRARY})
elseif (RL_BUILD AND ANDROID)
  target_link_libraries(rlgame PRIVATE android log)
elseif (RL_BUILD AND IOS)
  target_link_libraries(rlgame PRIVATE "-framework UIKit" "-framework Foundation" "-framework CoreFoundation")
endif ()

if(RL_BUILD)
  target_compile_definitions(
    rlgame
    PRIVATE
    ${EXTERN_IMPL}
  )
  target_compile_definitions(rlgame PRIVATE $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG>)
  apply_gpu_backend(rlgame)
  if (NOT MSVC)
    target_compile_options(rlgame PRIVATE $<$<CONFIG:Debug>:-fsanitize=address;-fno-omit-frame-pointer>)
    target_link_options(rlgame PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
  endif()
endif()
