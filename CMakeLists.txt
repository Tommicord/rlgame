cmake_minimum_required(VERSION 4.2.0)

project(rlgame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT)
if(IPO_SUPPORTED)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "Link Time Optimization (LTO) enabled")
else()
  message(WARNING "IPO/LTO is not supported: ${IPO_OUTPUT}")
endif()

if (WIN32)
  add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
  add_compile_definitions(VK_USE_PLATFORM_METAL_EXT)
elseif (UNIX)
  add_compile_definitions(VK_USE_PLATFORM_XCB_KHR)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-protocols wayland-cursor)
  if(WAYLAND_FOUND)
    add_compile_definitions(VK_USE_PLATFORM_WAYLAND_KHR)
  endif()
endif ()

add_compile_definitions(_R_CHUNK_VULKAN_BACKEND)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(R_DEVMODE R_CSTL_TRACE_ENABLED)
  message(STATUS "Development mode enabled")
  message(STATUS "Trace logging enabled")
endif ()

if (MSVC)
  add_compile_definitions(R_COMPILER_MSVC)
  message(STATUS "Compiler: MSVC detected")
elseif (CMAKE_C_COMPILER_ID MATCHES "GNU")
  add_compile_definitions(R_COMPILER_GCC)
  message(STATUS "Compiler: GCC detected")
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_definitions(R_COMPILER_CLANG)
  message(STATUS "Compiler: Clang detected")
else ()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
  message(STATUS "Target architecture ARM; NEON SIMD enabled")
  add_compile_definitions(R_SIMD_ARM_NEON)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
  message(STATUS "Target architecture x86/x86-64")
  
  include(CheckCSourceCompiles)
  check_c_source_compiles("
    #include <immintrin.h>
    int main() {
      __m256i test = _mm256_set1_epi32(0);
      return 0;
    }
  " HAS_AVX2)
  
  if (HAS_AVX2)
    message(STATUS "AVX2 SIMD enabled")
    add_compile_definitions(R_SIMD_AVX2)
    if(MSVC)
      add_compile_options(/arch:AVX2)
    else()
      add_compile_options(-mavx2)
    endif()
  else()
    # Check for SSE support
    check_c_source_compiles("
      #include <immintrin.h>
      int main() {
        __m128i test = _mm_set1_epi32(0);
        return 0;
      }
    " HAS_SSE)
    
    if (HAS_SSE)
      message(STATUS "SSE SIMD enabled")
      add_compile_definitions(R_SIMD_SSE)
      if(MSVC)
        add_compile_options(/arch:SSE2)
      else()
        add_compile_options(-msse2)
      endif()
    else()
      message(WARNING "No SIMD support detected for x86 architecture")
    endif()
  endif()
else ()
  message(WARNING "Unknown target architecture ${CMAKE_SYSTEM_PROCESSOR}; SIMD optimizations disabled")
endif ()

find_package(Vulkan REQUIRED COMPONENTS glslc)

# Find OpenCL for GPU memory defragmentation (fallback for non-NVIDIA GPUs)
find_package(OpenCL)
if(OpenCL_FOUND)
  message(STATUS "OpenCL found - GPU defragmentation support enabled")
  add_compile_definitions(R_CVULKAN_DEFRAG_OPENCL_ENABLED)
else()
  message(STATUS "OpenCL not found - GPU defragmentation support limited to CUDA")
endif()

# Find CUDA for NVIDIA GPU memory defragmentation (preferred for NVIDIA GPUs)
find_package(CUDA)
if(CUDA_FOUND)
  message(STATUS "CUDA found - NVIDIA GPU defragmentation support enabled")
  add_compile_definitions(R_CVULKAN_DEFRAG_CUDA_ENABLED)
  enable_language(CUDA)
else()
  message(STATUS "CUDA not found - NVIDIA GPU defragmentation support disabled")
endif()

find_program(RENDERDOC_EXE renderdoccmd DOC "RenderDoc command line tool")
if(RENDERDOC_EXE)
  message(STATUS "RenderDoc found: ${RENDERDOC_EXE}")
  set(RENDERDOC_FOUND TRUE)
else()
  message(STATUS "RenderDoc not found in PATH; RenderDoc integration disabled")
  set(RENDERDOC_FOUND FALSE)
endif()

function(target_add_renderdoc TARGET)
  if(RENDERDOC_FOUND AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${TARGET} PRIVATE _R_RENDERDOC_ENABLE)
    
    get_filename_component(RENDERDOC_DIR "${RENDERDOC_EXE}" ABSOLUTE PATH)
    get_filename_component(RENDERDOC_DIR "${RENDERDOC_DIR}" ABSOLUTE PATH)
    get_filename_component(RENDERDOC_DIR "${RENDERDOC_DIR}" DIRECTORY)

    set(RENDERDOC_INCLUDE_DIR "${RENDERDOC_DIR}")
    message(LOG "RenderDoc include directory set to: ${RENDERDOC_INCLUDE_DIR}")
    
    if(EXISTS "${RENDERDOC_INCLUDE_DIR}")
      target_include_directories(${TARGET} PRIVATE "${RENDERDOC_INCLUDE_DIR}")
      message(STATUS "RenderDoc integration enabled for target: ${TARGET}")
      message(STATUS "RenderDoc include directory: ${RENDERDOC_INCLUDE_DIR}")
    else()
      message(WARNING "RenderDoc include directory not found at: ${RENDERDOC_INCLUDE_DIR}")
    endif()
  endif()
endfunction()

function(set_common_output_directories TARGET)
  set_target_properties(
    ${TARGET}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release
  )
endfunction()

function(set_base_include_directories TARGET)
  target_include_directories(
    ${TARGET}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/.shader
  )
endfunction()

function(link_base_libraries EXECUTABLE)
  set(LIBRARIES 
    rlgame.base.cstl 
    rlgame.base.cvulkan 
    rlgame.base.game 
    rlgame.base.entry)
  target_link_libraries(${EXECUTABLE} PRIVATE ${LIBRARIES})
endfunction()

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

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(deps/gtest)
include(GoogleTest)

file(GLOB_RECURSE CSTL_SOURCES    CONFIGURE_DEPENDS src/rlgame.base/cstl/*.c)
file(GLOB_RECURSE CSTL_HEADERS    CONFIGURE_DEPENDS src/rlgame.base/cstl/*.h)
file(GLOB_RECURSE CVULKAN_SOURCES CONFIGURE_DEPENDS src/rlgame.base/cvulkan/*.c)
file(GLOB_RECURSE CVULKAN_HEADERS CONFIGURE_DEPENDS src/rlgame.base/cvulkan/*.h)
file(GLOB_RECURSE CVULKAN_CUDA_SOURCES CONFIGURE_DEPENDS src/rlgame.base/cvulkan/*.cu)
file(GLOB_RECURSE GAME_SOURCES    CONFIGURE_DEPENDS src/rlgame.base/game/*.c)
file(GLOB_RECURSE GAME_HEADERS    CONFIGURE_DEPENDS src/rlgame.base/game/*.h)
file(GLOB         MAIN_SOURCES    CONFIGURE_DEPENDS src/rlgame.base/*.c)
file(GLOB         MAIN_HEADERS    CONFIGURE_DEPENDS src/rlgame.base/*.h)
list(FILTER MAIN_SOURCES EXCLUDE REGEX "main\\.(c|h)$")

add_library(rlgame.base.cstl SHARED ${CSTL_SOURCES} ${CSTL_HEADERS})

if (WIN32)
  target_link_libraries(rlgame.base.cstl PUBLIC dbghelp)
endif()

target_compile_definitions(rlgame.base.cstl PRIVATE $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG> R_CSTL_BUILDING_DLL)
set_base_include_directories(rlgame.base.cstl)

add_library(rlgame.base.cvulkan SHARED ${CVULKAN_SOURCES} ${CVULKAN_HEADERS} ${CVULKAN_CUDA_SOURCES})

set_common_output_directories(rlgame.base.cvulkan)
set_base_include_directories(rlgame.base.cvulkan)
target_compile_definitions(rlgame.base.cvulkan PUBLIC $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG> R_CVULKAN_BUILDING_DLL)

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

if (UNIX AND NOT APPLE)
  target_link_options(rlgame.base.cvulkan PUBLIC $<$<CONFIG:Debug>:-rdynamic>)
endif()

add_library(rlgame.base.game SHARED ${GAME_SOURCES} ${GAME_HEADERS})

set_common_output_directories(rlgame.base.game)
set_base_include_directories(rlgame.base.game)
target_compile_definitions(rlgame.base.game PUBLIC $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG> R_GAME_BUILDING_DLL)

target_link_libraries(
  rlgame.base.game
  PUBLIC
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

if (APPLE)
  find_package(MoltenVK QUIET)
  if (MoltenVK_FOUND)
    target_link_libraries(rlgame.base.game PUBLIC MoltenVK::MoltenVK)
    target_compile_definitions(rlgame.base.game PUBLIC VK_USE_PLATFORM_METAL_EXT)
  endif()
endif()

set(MAIN_SOURCE src/rlgame.base/main.c)
if(NOT WIN32)
  add_executable(rlgame ${MAIN_SOURCES} ${MAIN_HEADERS})
else()
  add_executable(rlgame WIN32 ${MAIN_SOURCE})
endif()

link_base_libraries(rlgame)

if (UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
  target_link_libraries(rlgame PRIVATE X11::X11 dl)
  if(WAYLAND_FOUND)
    target_link_libraries(rlgame PRIVATE ${WAYLAND_LIBRARIES})
    target_include_directories(rlgame PRIVATE ${WAYLAND_INCLUDE_DIRS})
    target_compile_options(rlgame PRIVATE ${WAYLAND_CFLAGS_OTHER})
  endif()
elseif (APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)
  find_library(FOUNDATION_LIBRARY Foundation)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  target_link_libraries(rlgame PRIVATE ${COCOA_LIBRARY} ${APPLICATIONSERVICES_LIBRARY} ${FOUNDATION_LIBRARY} ${COREFOUNDATION_LIBRARY})
elseif (ANDROID)
  target_link_libraries(rlgame PRIVATE android log)
elseif (IOS)
  target_link_libraries(rlgame PRIVATE "-framework UIKit" "-framework Foundation" "-framework CoreFoundation")
endif ()

set(SHADERS
  src/rlgame.shader/TestTriangle.frag
  src/rlgame.shader/TestTriangle.vert
  src/rlgame.shader/TestTriangle3d.frag
  src/rlgame.shader/TestTriangle3d.vert
  src/rlgame.shader/Simplex.comp
  src/rlgame.shader/WorldHeightmap.comp
  src/rlgame.shader/WorldClimate.comp
  src/rlgame.shader/WorldUnitPlace.comp
  src/rlgame.shader/WorldMeshGen.comp
  src/rlgame.shader/WorldMeshGen.vert
  src/rlgame.shader/WorldMeshGen.frag
  src/rlgame.shader/WorldMeshGenGeom.comp
  src/rlgame.shader/WorldMeshDelaunay2D.comp
  src/rlgame.shader/WorldMeshGenGeomNoOcclusion.comp
  src/rlgame.shader/WorldOcclusionCull.comp
)

set(CUDA_FILES
  src/rlgame.base/cvulkan/cvulkan_defragmentation.cu
)
set(OPENCL_FILES
  src/rlgame.base/cvulkan/cvulkan_defragmentation.cl
)

enable_testing()

file(
  GLOB_RECURSE 
    TEST_SOURCES 
  CONFIGURE_DEPENDS 
    tests/*.cpp
)

add_executable(rlgame_test ${TEST_SOURCES})

target_include_directories(
  rlgame_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/stb
)

target_link_libraries(
  rlgame_test
  PRIVATE
  GTest::gtest_main
  rlgame.base.cstl
  rlgame.base.cvulkan
)
target_compile_definitions(rlgame_test PRIVATE $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG>)

if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(rlgame_test PRIVATE /profile)
  target_link_options(rlgame_test PRIVATE /PROFILE)
  
  target_compile_options(rlgame.base.cstl PRIVATE /profile)
  target_link_options(rlgame.base.cstl PRIVATE /PROFILE)
  message(STATUS "Code coverage enabled for MSVC Debug build")
endif()

# target_add_renderdoc(rlgame_test)

# Platform-specific libraries for test target
if (UNIX AND NOT APPLE)
  if(WAYLAND_FOUND)
    target_link_libraries(rlgame_test PRIVATE ${WAYLAND_LIBRARIES})
    target_include_directories(rlgame_test PRIVATE ${WAYLAND_INCLUDE_DIRS})
    target_compile_options(rlgame_test PRIVATE ${WAYLAND_CFLAGS_OTHER})
  endif()
elseif (APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)
  find_library(FOUNDATION_LIBRARY Foundation)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  target_link_libraries(rlgame_test PRIVATE ${COCOA_LIBRARY} ${APPLICATIONSERVICES_LIBRARY} ${FOUNDATION_LIBRARY} ${COREFOUNDATION_LIBRARY})
elseif (ANDROID)
  target_link_libraries(rlgame_test PRIVATE android log)
elseif (IOS)
  target_link_libraries(rlgame_test PRIVATE "-framework UIKit" "-framework Foundation" "-framework CoreFoundation")
elseif (WIN32)
  target_link_libraries(rlgame_test PRIVATE dbghelp)
endif ()

set_common_output_directories(rlgame_test)
gtest_discover_tests(rlgame_test)

file(
  GLOB_RECURSE
    BENCH_SOURCES
  CONFIGURE_DEPENDS
    benchmarks/*.cpp
)

add_executable(rlgame_bench ${BENCH_SOURCES})

target_include_directories(
  rlgame_bench
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
  rlgame_bench
  PRIVATE
  benchmark::benchmark
  benchmark::benchmark_main
  rlgame.base.cstl
  rlgame.base.cvulkan
)

set_common_output_directories(rlgame_bench)

target_compile_definitions(
  rlgame
  PRIVATE
  ${EXTERN_IMPL}
)

# Apply debug macro and ASan flags to executable and test target as well
target_compile_definitions(rlgame PRIVATE $<$<CONFIG:Debug>:R_CSTL_HEAP_DEBUG>)
if (NOT MSVC)
  target_compile_options(rlgame PRIVATE $<$<CONFIG:Debug>:-fsanitize=address;-fno-omit-frame-pointer>)
  target_link_options(rlgame PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
endif()

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

set(SPV_OUTPUTS "")
set(EMBEDDED_HEADERS "")
set(EMBEDDED_CFILES "")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GLSLC_DEBUG_FLAGS -g)
else()
  set(GLSLC_DEBUG_FLAGS "")
endif()

set(UUID_NAMESPACE "6ba7b810-9dad-11d1-80b4-00c04fd430c8")
foreach (SHADER IN LISTS SHADERS)
  get_filename_component(
    SHADER_ABS_PATH
    "${SHADER}"
    ABSOLUTE)
  get_filename_component(
    SHADER_NAME
    "${SHADER}"
    NAME)
  get_filename_component(
    SHADER_NAME_WE
    "${SHADER}"
    NAME_WE)
  get_filename_component(
    SHADER_EXT
    "${SHADER}"
    EXT)
  string(REPLACE "." "_" SHADER_VAR_NAME "${SHADER_NAME}")

  string(RANDOM LENGTH 32 RANDOM_NAME)
  string(
    UUID EMBEDDED_UUID
    NAMESPACE ${UUID_NAMESPACE} 
    NAME ${RANDOM_NAME} 
    TYPE SHA1
    UPPER
  )
  
  set(SPV "${CMAKE_CURRENT_BINARY_DIR}/.shader/spv/${SHADER_NAME}.spv")
  set(EMBEDDED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/.shader/${RANDOM_NAME}.h")
  set(EMBEDDED_CFILE "${CMAKE_CURRENT_BINARY_DIR}/.shader/${RANDOM_NAME}.cpp")
  
  add_custom_command(
    OUTPUT "${SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/.shader/spv/"
    COMMAND Vulkan::glslc ${GLSLC_DEBUG_FLAGS} "${SHADER_ABS_PATH}" -o "${SPV}"
    DEPENDS "${SHADER_ABS_PATH}"
    COMMENT "Compiling shader ${SHADER_NAME}"
    VERBATIM
  )
  
  add_custom_command(
    OUTPUT "${EMBEDDED_HEADER}" "${EMBEDDED_CFILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/.shader/"
    COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${SPV} -DOUTPUT_H=${EMBEDDED_HEADER} -DOUTPUT_C=${EMBEDDED_CFILE} -DVARIABLE_NAME=${SHADER_VAR_NAME} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinary.cmake"
    DEPENDS "${SPV}"
    COMMENT "Embedding ${SHADER_NAME}"
    VERBATIM
  )
  
  list(APPEND SPV_OUTPUTS "${SPV}")
  list(APPEND EMBEDDED_HEADERS "${EMBEDDED_HEADER}")
  list(APPEND EMBEDDED_CFILES "${EMBEDDED_CFILE}")
endforeach ()

# Embed CUDA files for GPU memory defragmentation
if(CUDA_FOUND)
  foreach (CUDA_FILE IN LISTS CUDA_FILES)
    get_filename_component(
      CUDA_ABS_PATH
      "${CUDA_FILE}"
      ABSOLUTE)
    get_filename_component(
      CUDA_NAME
      "${CUDA_FILE}"
      NAME)
    get_filename_component(
      CUDA_NAME_WE
      "${CUDA_FILE}"
      NAME_WE)
    string(REPLACE "." "_" CUDA_VAR_NAME "${CUDA_NAME_WE}")

    string(RANDOM LENGTH 32 RANDOM_NAME)
    string(
      UUID EMBEDDED_UUID
      NAMESPACE ${UUID_NAMESPACE} 
      NAME ${RANDOM_NAME} 
      TYPE SHA1
      UPPER
    )
    
    set(CUBIN "${CMAKE_CURRENT_BINARY_DIR}/.cuda/${CUDA_NAME_WE}.cubin")
    set(EMBEDDED_CUDA_HEADER "${CMAKE_CURRENT_BINARY_DIR}/.cuda/${RANDOM_NAME}.h")
    set(EMBEDDED_CUDA_CFILE "${CMAKE_CURRENT_BINARY_DIR}/.cuda/${RANDOM_NAME}.cpp")
    
    add_custom_command(
      OUTPUT "${CUBIN}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/.cuda/"
      COMMAND CUDA::nvcc --cubin -o "${CUBIN}" "${CUDA_ABS_PATH}"
      DEPENDS "${CUDA_ABS_PATH}"
      COMMENT "Compiling CUDA file ${CUDA_NAME}"
      VERBATIM
    )
    
    add_custom_command(
      OUTPUT "${EMBEDDED_CUDA_HEADER}" "${EMBEDDED_CUDA_CFILE}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/.cuda/"
      COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${CUBIN} -DOUTPUT_H=${EMBEDDED_CUDA_HEADER} -DOUTPUT_C=${EMBEDDED_CUDA_CFILE} -DVARIABLE_NAME=${CUDA_VAR_NAME} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedCuda.cmake"
      DEPENDS "${CUBIN}"
      COMMENT "Embedding CUDA ${CUDA_NAME}"
      VERBATIM
    )
    
    list(APPEND CUDA_OUTPUTS "${CUBIN}")
    list(APPEND EMBEDDED_CUDA_HEADERS "${EMBEDDED_CUDA_HEADER}")
    list(APPEND EMBEDDED_CUDA_CFILES "${EMBEDDED_CUDA_CFILE}")
  endforeach ()
endif()

# Embed OpenCL files for GPU memory defragmentation
if(OpenCL_FOUND)
  foreach (OPENCL_FILE IN LISTS OPENCL_FILES)
    get_filename_component(
      OPENCL_ABS_PATH
      "${OPENCL_FILE}"
      ABSOLUTE)
    get_filename_component(
      OPENCL_NAME
      "${OPENCL_FILE}"
      NAME)
    get_filename_component(
      OPENCL_NAME_WE
      "${OPENCL_FILE}"
      NAME_WE)
    string(REPLACE "." "_" OPENCL_VAR_NAME "${OPENCL_NAME_WE}")

    string(RANDOM LENGTH 32 RANDOM_NAME)
    string(
      UUID EMBEDDED_UUID
      NAMESPACE ${UUID_NAMESPACE} 
      NAME ${RANDOM_NAME} 
      TYPE SHA1
      UPPER
    )
    
    set(EMBEDDED_OPENCL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/.opencl/${RANDOM_NAME}.h")
    set(EMBEDDED_OPENCL_CFILE "${CMAKE_CURRENT_BINARY_DIR}/.opencl/${RANDOM_NAME}.cpp")
    
    add_custom_command(
      OUTPUT "${EMBEDDED_OPENCL_HEADER}" "${EMBEDDED_OPENCL_CFILE}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/.opencl/"
      COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${OPENCL_ABS_PATH} -DOUTPUT_H=${EMBEDDED_OPENCL_HEADER} -DOUTPUT_C=${EMBEDDED_OPENCL_CFILE} -DVARIABLE_NAME=${OPENCL_VAR_NAME} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinary.cmake"
      DEPENDS "${OPENCL_ABS_PATH}"
      COMMENT "Embedding OpenCL ${OPENCL_NAME}"
      VERBATIM
    )
    
    list(APPEND EMBEDDED_OPENCL_HEADERS "${EMBEDDED_OPENCL_HEADER}")
    list(APPEND EMBEDDED_OPENCL_CFILES "${EMBEDDED_OPENCL_CFILE}")
  endforeach ()
endif()

target_sources(rlgame PRIVATE ${EMBEDDED_HEADERS})
target_sources(rlgame PRIVATE ${EMBEDDED_CFILES})
if(CUDA_FOUND)
  target_sources(rlgame PRIVATE ${EMBEDDED_CUDA_HEADERS})
  target_sources(rlgame PRIVATE ${EMBEDDED_CUDA_CFILES})
endif()
if(OpenCL_FOUND)
  target_sources(rlgame PRIVATE ${EMBEDDED_OPENCL_HEADERS})
  target_sources(rlgame PRIVATE ${EMBEDDED_OPENCL_CFILES})
endif()