find_package(Vulkan REQUIRED COMPONENTS glslc)

# Wayland protocols (for Linux Wayland support)
if(UNIX AND NOT APPLE)
  find_package( PkgConfig)
  if( PkgConfig_FOUND)
    pkg_check_modules(WAYLAND_CLIENT wayland-client)
    pkg_check_modules(WAYLAND_PROTOCOLS wayland-protocols)

    # XCB for X11 support (XCB is preferred over Xlib)
    pkg_check_modules(XCB xcb)
    pkg_check_modules(XCB_XINPUT xcb-xinput)

    # Xlib for X11 support (legacy, used by some backends)
    pkg_check_modules(X11 x11)
    
    if(WAYLAND_CLIENT_FOUND)
      message(STATUS "Wayland client found")
      add_compile_definitions(R_CVULKAN_PLATFORM_LINUX)
    else()
      message(STATUS "Wayland client not found")
    endif()

    if(XCB_FOUND)
      message(STATUS "XCB found")
      add_compile_definitions(R_CVULKAN_PLATFORM_LINUX)
    else()
      message(STATUS "XCB not found")
    endif()

    # Find wayland-scanner
    find_program(WAYLAND_SCANNER wayland-scanner)
    if(WAYLAND_SCANNER)
      message(STATUS "Wayland scanner found: ${WAYLAND_SCANNER}")

      # Find xdg-shell.xml
      find_file(XDG_SHELL_XML xdg-shell.xml
        PATHS /usr/share/wayland-protocols/stable/xdg-shell/
              /usr/local/share/wayland-protocols/stable/xdg-shell/
        NO_DEFAULT_PATH)

      if(XDG_SHELL_XML)
        message(STATUS "xdg-shell.xml found: ${XDG_SHELL_XML}")

        # Generate xdg-shell.h and xdg-shell.c
        set(XDG_SHELL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/third_party/wayland)
        file(MAKE_DIRECTORY ${XDG_SHELL_OUTPUT_DIR})
        set(XDG_SHELL_HEADER ${XDG_SHELL_OUTPUT_DIR}/xdg-shell.h)
        set(XDG_SHELL_CODE ${XDG_SHELL_OUTPUT_DIR}/xdg-shell.c)

        add_custom_command(
          OUTPUT ${XDG_SHELL_HEADER}
          COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${XDG_SHELL_HEADER}
          DEPENDS ${XDG_SHELL_XML}
          COMMENT "Generating xdg-shell.h from ${XDG_SHELL_XML}"
        )

        add_custom_command(
          OUTPUT ${XDG_SHELL_CODE}
          COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${XDG_SHELL_CODE}
          DEPENDS ${XDG_SHELL_XML}
          COMMENT "Generating xdg-shell.c from ${XDG_SHELL_XML}"
        )

        add_library(wayland_xdg_shell STATIC ${XDG_SHELL_CODE} ${XDG_SHELL_HEADER})
        target_include_directories(wayland_xdg_shell PUBLIC ${XDG_SHELL_OUTPUT_DIR})
        target_link_libraries(wayland_xdg_shell PUBLIC ${WAYLAND_CLIENT_LIBRARIES})
      else()
        message(WARNING "xdg-shell.xml not found")
      endif()
    else()
      message(WARNING "wayland-scanner not found")
    endif()
  else()
    message(STATUS "Wayland client not found")
  endif()
endif()

# vcpkg integration: use vcpkg toolchain to install OpenCL automatically
# Run: cmake --preset default (or release) to use vcpkg toolchain
find_package(OpenCL CONFIG QUIET)
if(OpenCL_FOUND)
  message(STATUS "OpenCL found - GPU parallel computing enabled")
else()
  message(STATUS "OpenCL not found - GPU parallel computing support limited to CUDA")
  message(STATUS "  To enable OpenCL via vcpkg:")
  message(STATUS "    1. Install vcpkg: https://github.com/microsoft/vcpkg")
  message(STATUS "    2. Run: vcpkg install opencl:x64-windows")
  message(STATUS "    3. Settingsure with: cmake --preset default")
endif()

find_package(CUDA CONFIG)
if(CUDA_FOUND)
  message(STATUS "CUDA found - NVIDIA GPU parallel computing support enabled")
  enable_language(CUDA)
else()
  message(STATUS "CUDA not found - NVIDIA GPU parallel computing support disabled")
  set(CVULKAN_CUDA_SOURCES)
endif()

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_USE_BUNDLED_GTEST OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

# Google Test
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/googletest)
add_subdirectory(third_party/googlebenchmark)
include(GoogleTest)
