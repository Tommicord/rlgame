if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
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
    rlgame.base.rpack
    rlgame.microbit
  )
  target_compile_definitions(rlgame_test PRIVATE R_CSTL_HEAP_DEBUG)

  set_common_output_directories(rlgame_test)
  gtest_discover_tests(rlgame_test)
endif()

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
