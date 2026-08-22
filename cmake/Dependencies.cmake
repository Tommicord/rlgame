find_package(Vulkan REQUIRED COMPONENTS glslc)

# vcpkg integration: use vcpkg toolchain to install OpenCL automatically
# Run: cmake --preset default (or release) to use vcpkg toolchain
find_package(OpenCL QUIET)
if(OpenCL_FOUND)
  message(STATUS "OpenCL found - GPU defragmentation support enabled")
  message(STATUS "  OpenCL include dirs: ${OpenCL_INCLUDE_DIRS}")
  message(STATUS "  OpenCL libraries: ${OpenCL_LIBRARIES}")
  add_compile_definitions(R_CVULKAN_DEFRAG_OPENCL_ENABLED)
else()
  message(STATUS "OpenCL not found - GPU defragmentation support limited to CUDA")
  message(STATUS "  To enable OpenCL via vcpkg:")
  message(STATUS "    1. Install vcpkg: https://github.com/microsoft/vcpkg")
  message(STATUS "    2. Run: vcpkg install opencl:x64-windows")
  message(STATUS "    3. Configure with: cmake --preset default")
endif()

# CUDA for NVIDIA GPU memory defragmentation (preferred for NVIDIA GPUs)
find_package(CUDA)
if(CUDA_FOUND)
  message(STATUS "CUDA found - NVIDIA GPU defragmentation support enabled")
  add_compile_definitions(R_CVULKAN_DEFRAG_CUDA_ENABLED)
  enable_language(CUDA)
else()
  message(STATUS "CUDA not found - NVIDIA GPU defragmentation support disabled")
endif()

# RenderDoc (optional)
find_program(RENDERDOC_EXE renderdoccmd DOC "RenderDoc command line tool")
if(RENDERDOC_EXE)
  message(STATUS "RenderDoc found: ${RENDERDOC_EXE}")
  set(RENDERDOC_FOUND TRUE)
else()
  message(STATUS "RenderDoc not found in PATH; RenderDoc integration disabled")
  set(RENDERDOC_FOUND FALSE)
endif()

# Google Benchmark
include(FetchContent)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.2
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_USE_BUNDLED_GTEST OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# Google Test
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(deps/gtest)
include(GoogleTest)
