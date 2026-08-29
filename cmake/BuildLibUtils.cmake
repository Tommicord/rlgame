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
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release
  )
endfunction()

# Function to set base include directories for a target
function(set_base_include_directories TARGET)
  target_include_directories(
    ${TARGET}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/consoletools
  )
endfunction()

# Function to link base libraries to an executable
function(link_base_libraries EXECUTABLE)
  set(LIBRARIES 
    rlgame.base.cstl 
    rlgame.base.cvulkan 
    rlgame.base.game 
    rlgame.client.render
    rlgame.microbit
    rlgame.base.entry)
  target_link_libraries(${EXECUTABLE} PRIVATE ${LIBRARIES})
endfunction()

function(link_wayland TARGET)
  if(UNIX AND NOT APPLE AND WAYLAND_CLIENT_FOUND)
    target_link_libraries(${TARGET} PRIVATE wayland_xdg_shell ${WAYLAND_CLIENT_LIBRARIES})
    target_include_directories(${TARGET} PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
    target_compile_options(${TARGET} PRIVATE ${WAYLAND_CLIENT_CFLAGS_OTHER})

    pkg_check_modules(WAYLAND_PROTOCOLS_LIB wayland-protocols IMPORTED_TARGET)
    if(WAYLAND_PROTOCOLS_LIB_FOUND)
      target_link_libraries(${TARGET} PRIVATE PkgConfig::WAYLAND_PROTOCOLS_LIB)
    endif()
  endif()
endfunction()

# Function to link XCB libraries to a target
function(link_xcb TARGET)
  if(UNIX AND NOT APPLE AND XCB_FOUND)
    target_link_libraries(${TARGET} PRIVATE ${XCB_LIBRARIES})
    if(XCB_XINPUT_FOUND)
      target_link_libraries(${TARGET} PRIVATE ${XCB_XINPUT_LIBRARIES})
    endif()
    target_include_directories(${TARGET} PRIVATE ${XCB_INCLUDE_DIRS})
    target_compile_options(${TARGET} PRIVATE ${XCB_CFLAGS_OTHER})
  endif()
endfunction()

# Function to link X11 (Xlib) libraries to a target
function(link_x11 TARGET)
  if(UNIX AND NOT APPLE AND X11_FOUND)
    target_link_libraries(${TARGET} PRIVATE ${X11_LIBRARIES})
    target_include_directories(${TARGET} PRIVATE ${X11_INCLUDE_DIRS})
    target_compile_options(${TARGET} PRIVATE ${X11_CFLAGS_OTHER})
  endif()
endfunction()

# Function to add resource files to a target
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
