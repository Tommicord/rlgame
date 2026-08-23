function(embedding_uuid OUTPUT_UUID)
  string(RANDOM LENGTH 8 ALPHABET "0123456789ABCDEF" BLOCK1)
  string(RANDOM LENGTH 4 ALPHABET "0123456789ABCDEF" BLOCK2)
  string(RANDOM LENGTH 4 ALPHABET "0123456789ABCDEF" BLOCK3)
  string(RANDOM LENGTH 4 ALPHABET "0123456789ABCDEF" BLOCK4)
  string(RANDOM LENGTH 12 ALPHABET "0123456789ABCDEF" BLOCK5)
  set(${OUTPUT_UUID} "${BLOCK1}-${BLOCK2}-${BLOCK3}-${BLOCK4}-${BLOCK5}" PARENT_SCOPE)
endfunction()

function(embed_file FILE_PATH OUTPUT_DIR VAR_NAME OUTPUTS_LIST HEADERS_LIST CFILES_LIST)
  get_filename_component(FILE_ABS_PATH "${FILE_PATH}" ABSOLUTE)
  get_filename_component(FILE_NAME "${FILE_PATH}" NAME)
  get_filename_component(FILE_NAME_WE "${FILE_PATH}" NAME_WE)
  get_filename_component(FILE_EXT "${FILE_PATH}" EXT)
  string(REPLACE "." "_" FILE_EXT_UNDERSCORE "${FILE_EXT}")

  embedding_uuid(RANDOM_NAME)
  set(EMBEDDED_HEADER "${OUTPUT_DIR}/${RANDOM_NAME}.h")
  set(EMBEDDED_CFILE "${OUTPUT_DIR}/${RANDOM_NAME}.c")
  add_custom_command(
    OUTPUT "${EMBEDDED_HEADER}" "${EMBEDDED_CFILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${FILE_ABS_PATH} -DOUTPUT_H=${EMBEDDED_HEADER} -DOUTPUT_C=${EMBEDDED_CFILE} -DVARIABLE_NAME=${VAR_NAME} -P "${CMAKE_SOURCE_DIR}/cmake/EmbedBinary.cmake"
    DEPENDS "${FILE_ABS_PATH}" "${CMAKE_SOURCE_DIR}/cmake/EmbedBinary.cmake"
    COMMENT "Embedding ${RANDOM_NAME}"
    VERBATIM
  )

  list(APPEND ${OUTPUTS_LIST} "${EMBEDDED_HEADER}")
  list(APPEND ${HEADERS_LIST} "${EMBEDDED_HEADER}")
  list(APPEND ${CFILES_LIST} "${EMBEDDED_CFILE}")
  set(${OUTPUTS_LIST} "${${OUTPUTS_LIST}}" PARENT_SCOPE)
  set(${HEADERS_LIST} "${${HEADERS_LIST}}" PARENT_SCOPE)
  set(${CFILES_LIST} "${${CFILES_LIST}}" PARENT_SCOPE)
endfunction()

function(compile_and_embed FILES_LIST COMPILE_TARGET COMPILE_FLAGS OUTPUT_EXT OUTPUT_DIR OUTPUTS_LIST HEADERS_LIST CFILES_LIST)
  foreach(FILE IN LISTS FILES_LIST)
    get_filename_component(FILE_ABS_PATH "${FILE}" ABSOLUTE)
    get_filename_component(FILE_NAME "${FILE}" NAME)
    get_filename_component(FILE_NAME_WE "${FILE}" NAME_WE)
    get_filename_component(FILE_EXT "${FILE}" EXT)
    string(REPLACE "." "_" FILE_EXT_UNDERSCORE "${FILE_EXT}")
    set(VAR_NAME "${FILE_NAME_WE}${FILE_EXT_UNDERSCORE}")

    string(REPLACE "." "_" FILE_EXT_UNDERSCORE "${FILE_EXT}")
    set(COMPILED_OUTPUT_NAME "${FILE_NAME_WE}${FILE_EXT_UNDERSCORE}.${OUTPUT_EXT}")
    set(COMPILED_OUTPUT "${OUTPUT_DIR}/${COMPILED_OUTPUT_NAME}")

    add_custom_command(
      OUTPUT "${COMPILED_OUTPUT}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
      COMMAND ${CMAKE_COMMAND} -E echo "Compiling input:${FILE_ABS_PATH} -> compiled:${COMPILED_OUTPUT}"
      COMMAND $<TARGET_FILE:${COMPILE_TARGET}> ${COMPILE_FLAGS} "${FILE_ABS_PATH}" -o "${COMPILED_OUTPUT}"
      DEPENDS "${FILE_ABS_PATH}" "${CMAKE_SOURCE_DIR}/cmake/EmbedBinary.cmake"
      COMMENT "Compiling ${FILE_NAME}"
      VERBATIM
    )
    embedding_uuid(RANDOM_NAME)

    set(EMBEDDED_HEADER "${OUTPUT_DIR}/${RANDOM_NAME}.h")
    set(EMBEDDED_CFILE "${OUTPUT_DIR}/${RANDOM_NAME}.c")

    add_custom_command(
      OUTPUT "${EMBEDDED_HEADER}" "${EMBEDDED_CFILE}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
      COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${FILE_ABS_PATH} -DOUTPUT_H=${EMBEDDED_HEADER} -DOUTPUT_C=${EMBEDDED_CFILE} -DVARIABLE_NAME=${VAR_NAME} -P "${CMAKE_SOURCE_DIR}/cmake/EmbedBinary.cmake"
      DEPENDS "${COMPILED_OUTPUT}"
      COMMENT "Embedding ${RANDOM_NAME}"
      VERBATIM
    )

    list(APPEND ${OUTPUTS_LIST} "${COMPILED_OUTPUT}")
    list(APPEND ${HEADERS_LIST} "${EMBEDDED_HEADER}")
    list(APPEND ${CFILES_LIST} "${EMBEDDED_CFILE}")
    set(${OUTPUTS_LIST} "${${OUTPUTS_LIST}}" PARENT_SCOPE)
    set(${HEADERS_LIST} "${${HEADERS_LIST}}" PARENT_SCOPE)
    set(${CFILES_LIST} "${${CFILES_LIST}}" PARENT_SCOPE)
  endforeach()
endfunction()

set(SHADERS
  src/rlgame.shader/test_triangle.frag
  src/rlgame.shader/test_triangle.vert
  src/rlgame.shader/test_triangle3d.frag
  src/rlgame.shader/test_triangle3d.vert
)

set(CUDA_FILES
  src/rlgame.cssources/cvulkan_defragmentation.cu
)

set(OPENCL_FILES
  src/rlgame.cssources/cvulkan_defragmentation.cl
)

set(SPV_OUTPUTS "")
set(EMBEDDED_HEADERS "")
set(EMBEDDED_CFILES "")
set(CUDA_OUTPUTS "")
set(EMBEDDED_CUDA_HEADERS "")
set(EMBEDDED_CUDA_CFILES "")
set(CL_OUTPUTS "")
set(EMBEDDED_CL_HEADERS "")
set(EMBEDDED_CL_CFILES "")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GLSLC_DEBUG_FLAGS -g)
else()
  set(GLSLC_DEBUG_FLAGS "")
endif()

set(SPV_DIR "${CMAKE_CURRENT_BINARY_DIR}/.spv")
set(CUDA_DIR "${CMAKE_CURRENT_BINARY_DIR}/.cuda")
set(CL_DIR "${CMAKE_CURRENT_BINARY_DIR}/.cl")

file(MAKE_DIRECTORY "${SPV_DIR}")
file(MAKE_DIRECTORY "${CUDA_DIR}")
file(MAKE_DIRECTORY "${CL_DIR}")

file(GLOB OLD_SPV "${SPV_DIR}/*")
foreach(F IN LISTS OLD_SPV)
  file(REMOVE "${F}")
endforeach()
file(GLOB OLD_CUDA "${CUDA_DIR}/*")
foreach(F IN LISTS OLD_CUDA)
  file(REMOVE "${F}")
endforeach()
file(GLOB OLD_CL "${CL_DIR}/*")
foreach(F IN LISTS OLD_CL)
  file(REMOVE "${F}")
endforeach()

compile_and_embed(
  "${SHADERS}"
  Vulkan::glslc
  "${GLSLC_DEBUG_FLAGS}"
  "spv"
  ${SPV_DIR}
  SPV_OUTPUTS
  EMBEDDED_HEADERS
  EMBEDDED_CFILES
)

if(CUDA_FOUND)
  compile_and_embed(
    "${CUDA_FILES}"
    CUDA::nvcc
    "--cubin"
    "cubin"
    "${CUDA_DIR}"
    CUDA_OUTPUTS
    EMBEDDED_CUDA_HEADERS
    EMBEDDED_CUDA_CFILES
  )
endif()
if(OpenCL_FOUND)
  foreach(FILE IN LISTS OPENCL_FILES)
    get_filename_component(FILE_NAME_WE "${FILE}" NAME_WE)
    set(VAR_NAME "${FILE_NAME_WE}")
    
    embed_file(
      "${FILE}" 
      "${CL_DIR}" 
      ${VAR_NAME} 
      CL_OUTPUTS 
      EMBEDDED_CL_HEADERS 
      EMBEDDED_CL_CFILES
      )
  endforeach()
else()
  message(STATUS "OpenCL not found, skipping CL files")
endif()

message(STATUS "Embedded CL headers: ${EMBEDDED_CL_HEADERS}")
message(STATUS "Embedded CL cfiles: ${EMBEDDED_CL_CFILES}")

target_sources(rlgame PRIVATE ${EMBEDDED_HEADERS})
target_sources(rlgame PRIVATE ${EMBEDDED_CFILES})
if(CUDA_FOUND)
  target_sources(rlgame PRIVATE ${EMBEDDED_CUDA_HEADERS})
  target_sources(rlgame PRIVATE ${EMBEDDED_CUDA_CFILES})
endif()

if(OpenCL_FOUND)
  target_sources(rlgame.base.cvulkan PRIVATE ${EMBEDDED_CL_HEADERS})
  target_sources(rlgame.base.cvulkan PRIVATE ${EMBEDDED_CL_CFILES})
else()
  message(STATUS "OpenCL not found, skipping CL files")
endif()