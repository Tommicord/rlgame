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

# MSVC code coverage for Debug builds
if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(rlgame_test PRIVATE /profile)
  target_link_options(rlgame_test PRIVATE /PROFILE)
  
  target_compile_options(rlgame.base.cstl PRIVATE /profile)
  target_link_options(rlgame.base.cstl PRIVATE /PROFILE)
  message(STATUS "Code coverage enabled for MSVC Debug build")
endif()

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

# Benchmark executable
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
