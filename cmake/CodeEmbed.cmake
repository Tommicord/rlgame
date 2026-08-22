set(SHADERS
  src/rlgame.shader/TestTriangle.frag
  src/rlgame.shader/TestTriangle.vert
  src/rlgame.shader/TestTriangle3d.frag
  src/rlgame.shader/TestTriangle3d.vert
  src/rlgame.shader/WorldMeshGen.vert
  src/rlgame.shader/WorldMeshGen.frag
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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GLSLC_DEBUG_FLAGS -g)
else()
  set(GLSLC_DEBUG_FLAGS "")
endif()

set(UUID_NAMESPACE "6ba7b810-9dad-11d1-80b4-00c04fd430c8")

# Embed shader files
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
set(CUDA_OUTPUTS "")
set(EMBEDDED_CUDA_HEADERS "")
set(EMBEDDED_CUDA_CFILES "")

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
set(EMBEDDED_OPENCL_HEADERS "")
set(EMBEDDED_OPENCL_CFILES "")

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
    
    set(EMBEDDED_OPENCL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/.cl/${RANDOM_NAME}.h")
    set(EMBEDDED_OPENCL_CFILE "${CMAKE_CURRENT_BINARY_DIR}/.cl/${RANDOM_NAME}.cpp")
    
    add_custom_command(
      OUTPUT "${EMBEDDED_OPENCL_HEADER}" "${EMBEDDED_OPENCL_CFILE}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/.cl/"
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
