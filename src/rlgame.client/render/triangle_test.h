#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "rlgame.client/render/render_platform.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/game/game_cvulkan_pipeline.h"
#include "rlgame.base/game/game_renderer_subsystem.h"

/**
 * @brief Triangle test context
 */
struct R_TriangleTest_Context
{
        struct R_Game_PipelineContext pipelineContext;
        struct R_CVulkan_PipelineLayout*     pipelineLayout;
        struct R_CVulkan_Pipeline*           graphicsPipeline;
        struct R_GameRendererSubsystem*      pRendererSubsystem;
        uint32_t                             layerIndex;
};

/**
 * @brief Initialize the triangle test
 * @param pContext Pointer to context to initialize
 * @param pCreateInfo Pipeline context creation info
 * @return R_GAME_OK on success, error code otherwise
 */
R_RENDER_API enum R_GameError R_TriangleTestInitialize (
    struct R_TriangleTest_Context*                 pContext,
    const struct R_Game_PipelineContextCreateInfo* pCreateInfo);

/**
 * @brief Register the triangle test with the renderer subsystem
 * @param pContext Pointer to triangle test context
 * @param pSubsystem Pointer to renderer subsystem
 * @return R_GAME_OK on success, error code otherwise
 */
R_RENDER_API enum R_GameError R_TriangleTestRegisterWithRenderer (
    struct R_TriangleTest_Context*      pContext,
    struct R_GameRendererSubsystem*     pSubsystem);

/**
 * @brief Render a frame
 * @param pContext Pointer to triangle test context
 * @return R_GAME_OK on success, error code otherwise
 */
R_RENDER_API enum R_GameError R_TriangleTestRenderFrame (struct R_TriangleTest_Context* pContext);

/**
 * @brief Cleanup the triangle test
 * @param pContext Pointer to context to cleanup
 */
R_RENDER_API void R_TriangleTestCleanup (struct R_TriangleTest_Context* pContext);

/**
 * @brief Callback for rendering the triangle
 * @param pUserData Pointer to user data (triangle test context)
 * @param pResource Pointer to resource (command buffer)
 * @param resourceSize Size of resource
 */
R_RENDER_API void R_TriangleTestRenderCallback (
    void*        pUserData,
    const void*  pResource,
    const size_t resourceSize);

/**
 * @brief Callback for beginning the render pass
 * @param pUserData Pointer to user data (triangle test context)
 * @param pResource Pointer to resource (command buffer)
 * @param resourceSize Size of resource
 */
R_RENDER_API void R_TriangleTestBeforePassCallback (
    void*        pUserData,
    const void*  pResource,
    const size_t resourceSize);

/**
 * @brief Callback for ending the render pass
 * @param pUserData Pointer to user data (triangle test context)
 * @param pResource Pointer to resource (command buffer)
 * @param resourceSize Size of resource
 */
R_RENDER_API void R_TriangleTestAfterPassCallback (
    void*        pUserData,
    const void*  pResource,
    const size_t resourceSize);
