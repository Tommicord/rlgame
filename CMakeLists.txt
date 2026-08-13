cmake_minimum_required(VERSION 3.28)

project(rlgame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
  add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
  add_compile_definitions(VK_USE_PLATFORM_METAL_EXT)
elseif (UNIX)
  add_compile_definitions(VK_USE_PLATFORM_XCB_KHR)
endif ()

if (MSVC)
  add_compile_definitions(_RL_COMPILER_MSVC)
  message(STATUS "Compiler: MSVC detected")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_definitions(_RL_COMPILER_GCC)
  message(STATUS "Compiler: GCC detected")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_definitions(_RL_COMPILER_CLANG)
  message(STATUS "Compiler: Clang detected")
else ()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
  message(STATUS "Target architecture ARM; NEON SIMD enabled")
  add_compile_definitions(_RL_SIMD_ARM_NEON)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64|amd64|i386|i686")
  message(STATUS "Target architecture x86/x86-64")
  
  # Check for AVX2 support
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("
    #include <immintrin.h>
    int main() {
      __m256i test = _mm256_set1_epi32(0);
      return 0;
    }
  " HAS_AVX2)
  
  if (HAS_AVX2)
    message(STATUS "AVX2 SIMD enabled")
    add_compile_definitions(_RL_SIMD_AVX2)
    if(MSVC)
      add_compile_options(/arch:AVX2)
    else()
      add_compile_options(-mavx2)
    endif()
  else()
    # Check for SSE support
    check_cxx_source_compiles("
      #include <immintrin.h>
      int main() {
        __m128i test = _mm_set1_epi32(0);
        return 0;
      }
    " HAS_SSE)
    
    if (HAS_SSE)
      message(STATUS "SSE SIMD enabled")
      add_compile_definitions(_RL_SIMD_SSE)
      if(MSVC)
        add_compile_options(/arch:SSE2)
      else()
        add_compile_options(-msse2)
      endif()
    else()
      message(WARNING "No SIMD support detected for x86 architecture")
    endif()
  endif()
  
  # Always set x86 SIMD flag for generic x86 code
  add_compile_definitions(_RL_SIMD_X86)
else ()
  message(WARNING "Unknown target architecture ${CMAKE_SYSTEM_PROCESSOR}; SIMD optimizations disabled")
endif ()

find_package(Vulkan REQUIRED COMPONENTS glslc)

# Check for RenderDoc in PATH for debugging integration
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
    target_compile_definitions(${TARGET} PRIVATE _RL_RENDERDOC_ENABLE)
    
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

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(deps/gtest)
include(GoogleTest)

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS src/**/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS src/**/*.h)

list(FILTER ALL_SOURCES EXCLUDE REGEX ".*Main\\.cpp$")
add_library(rlgame_lib STATIC ${ALL_SOURCES} ${HEADERS})

target_include_directories(
  rlgame_lib
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/stb
  ${CMAKE_CURRENT_BINARY_DIR}/.shader
)
target_link_libraries(
  rlgame_lib
  PUBLIC
  Vulkan::Vulkan
)
target_add_renderdoc(rlgame_lib)

set(MAIN_SOURCE src/Rl.Base/Main.cpp)
if(NOT WIN32)
  add_executable(rlgame ${MAIN_SOURCE})
else()
  add_executable(rlgame WIN32 ${MAIN_SOURCE})
endif()

if(MSVC)
  # MSVC: Use /WHOLEARCHIVE to force inclusion of all object files
  target_link_libraries(rlgame PRIVATE rlgame_lib)
  target_link_options(rlgame PRIVATE "/WHOLEARCHIVE:rlgame_lib")
elseif(APPLE)
  # macOS: Use -force_load
  target_link_libraries(rlgame PRIVATE -force_load $<TARGET_FILE:rlgame_lib>)
elseif(UNIX)
  # Linux/Unix: Use -Wl,--whole-archive -Wl,--no-whole-archive
  target_link_libraries(rlgame PRIVATE -Wl,--whole-archive rlgame_lib -Wl,--no-whole-archive)
else()
  target_link_libraries(rlgame PRIVATE rlgame_lib)
endif()

if (UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
  target_link_libraries(rlgame PRIVATE X11::X11 dl)
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
elseif (WIN32)
  target_link_libraries(rlgame PRIVATE dbghelp)
endif ()

set(SHADERS
  src/Rl.Shader/Triangle.frag
  src/Rl.Shader/Triangle.vert
  src/Rl.Shader/Triangle3d.frag
  src/Rl.Shader/Triangle3d.vert
  src/Rl.Shader/Simplex.comp
  src/Rl.Shader/WorldHeightmap.comp
  src/Rl.Shader/WorldClimate.comp
  src/Rl.Shader/WorldUnitPlace.comp
  src/Rl.Shader/WorldMeshGen.comp
  src/Rl.Shader/WorldMeshGen.vert
  src/Rl.Shader/WorldMeshGen.frag
  src/Rl.Shader/WorldMeshTess.comp
  src/Rl.Shader/WorldMeshIndexDedup.comp
  src/Rl.Shader/WorldMeshTessNoOcclusion.comp
  src/Rl.Shader/WorldOcclusionCull.comp
  src/Rl.Shader/FullscreenQuad.vert
  src/Rl.Shader/FullscreenQuad.frag
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
  rlgame_lib
)
target_add_renderdoc(rlgame_test)

# Platform-specific libraries for test target
if (UNIX AND NOT APPLE)
  target_link_libraries(rlgame_test PRIVATE X11::X11 dl)
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

set_target_properties(
  rlgame_test 
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug
)

gtest_discover_tests(rlgame_test)

set(
  EXTERN_IMPL
  STB_IMAGE_IMPLEMENTATION
)

target_compile_definitions(
  rlgame
  PRIVATE
  ${EXTERN_IMPL}
)

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
set(EMBEDDED_CPPS "")

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

  string(RANDOM LENGTH 16 RANDOM_NAME)
  string(
    UUID EMBEDDED_UUID
    NAMESPACE ${UUID_NAMESPACE} 
    NAME ${RANDOM_NAME} 
    TYPE MD5
  )
  
  set(SPV "${CMAKE_CURRENT_BINARY_DIR}/.shader/spv/${SHADER_NAME}.spv")
  set(EMBEDDED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/.shader/${RANDOM_NAME}.h")
  set(EMBEDDED_CPP "${CMAKE_CURRENT_BINARY_DIR}/.shader/${RANDOM_NAME}.cpp")
  
  add_custom_command(
    OUTPUT "${SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/.shader/spv/"
    COMMAND Vulkan::glslc ${GLSLC_DEBUG_FLAGS} "${SHADER_ABS_PATH}" -o "${SPV}"
    DEPENDS "${SHADER_ABS_PATH}"
    COMMENT "Compiling shader ${SHADER_NAME}"
    VERBATIM
  )
  
  add_custom_command(
    OUTPUT "${EMBEDDED_HEADER}" "${EMBEDDED_CPP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/.shader/"
    COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${SPV} -DOUTPUT_H=${EMBEDDED_HEADER} -DOUTPUT_CPP=${EMBEDDED_CPP} -DVARIABLE_NAME=${SHADER_VAR_NAME} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinary.cmake"
    DEPENDS "${SPV}"
    COMMENT "Embedding ${SHADER_NAME}"
    VERBATIM
  )
  
  list(APPEND SPV_OUTPUTS "${SPV}")
  list(APPEND EMBEDDED_HEADERS "${EMBEDDED_HEADER}")
  list(APPEND EMBEDDED_CPPS "${EMBEDDED_CPP}")
endforeach ()

target_sources(rlgame_lib PRIVATE ${EMBEDDED_HEADERS})
target_sources(rlgame_lib PRIVATE ${EMBEDDED_CPPS})