set(RPACK_ASSETS_DIR "${CMAKE_SOURCE_DIR}/public" CACHE PATH "Directory containing RPACK image assets")
set(RPACK_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rpack" CACHE PATH "Directory for generated RPACK files")
set(RPACK_OUTPUT_NAME "assets.rpack" CACHE STRING "Base name of the generated RPACK file")

file(GLOB_RECURSE RPACK_ASSET_FILES CONFIGURE_DEPENDS ${RPACK_ASSETS_DIR}/*.rpka)
list(SORT RPACK_ASSET_FILES)
list(LENGTH RPACK_ASSET_FILES RPACK_ASSET_COUNT)

if(RPACK_ASSET_COUNT GREATER 0)
    set(RPACK_OUTPUT_FILE "${RPACK_OUTPUT_DIR}/${RPACK_OUTPUT_NAME}")
    get_filename_component(RPACK_OUTPUT_STEM "${RPACK_OUTPUT_NAME}" NAME_WE)
    set(RPACK_OUTPUT_FILES "${RPACK_OUTPUT_FILE}")
    foreach(MIPMAP_SIZE 64 32 16 8 4 2 1)
        list(APPEND RPACK_OUTPUT_FILES
                "${RPACK_OUTPUT_DIR}/${RPACK_OUTPUT_STEM}_${MIPMAP_SIZE}x${MIPMAP_SIZE}.rpack")
    endforeach()
    add_custom_command(
            OUTPUT ${RPACK_OUTPUT_FILES} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RPACK_OUTPUT_DIR}"
            COMMAND $<TARGET_FILE:rlgame_rpack>
            --mipmap
            --workers ${RPACK_ASSET_COUNT}
            --output "${RPACK_OUTPUT_FILE}"
            ${RPACK_ASSET_FILES}
            DEPENDS rlgame_rpack ${RPACK_ASSET_FILES}
            COMMENT "Packing ${RPACK_ASSET_COUNT} image asset(s) with ${RPACK_ASSET_COUNT} worker(s)"
            VERBATIM
    )
    add_custom_target(rlgame_rpack_assets DEPENDS ${RPACK_OUTPUT_FILES})
    message(STATUS "RPACK assets: ${RPACK_ASSET_COUNT} image(s), ${RPACK_ASSET_COUNT} worker(s)")
else()
    add_custom_target(
            rlgame_rpack_assets
            COMMAND ${CMAKE_COMMAND} -E echo "No assets found in ${RPACK_ASSETS_DIR}; rlgame_rpack_assets skipped"
            VERBATIM
    )
    message(STATUS "RPACK assets: no .RPKA files found in ${RPACK_ASSETS_DIR}")
endif()
