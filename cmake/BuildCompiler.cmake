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
  add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(X11 REQUIRED x11)
endif ()

add_compile_definitions(_R_CHUNK_VULKAN_BACKEND)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(R_DEVMODE R_CSTL_TRACE_ENABLED)
  message(STATUS "Development mode enabled")
  message(STATUS "Trace logging enabled")
endif ()

# Compiler detection
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

# Architecture and SIMD detection
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
