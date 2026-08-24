#include "rlgame.client/render/render_platform.h"
#include "rlgame.client/render/triangle_test.h"
#include "rlgame.base/cvulkan/cvulkan_instance.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_pipeline.h"
#include "rlgame.base/cvulkan/cvulkan_shader_module.h"
#include "rlgame.base/cvulkan/cvulkan_swapchain.h"
#include "rlgame.base/cvulkan/cvulkan_command_buffer.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>
#include <stdio.h>

extern const uint32_t testTriangleVert_size;
extern const uint32_t testTriangleVert_data[];
extern const uint32_t testTriangleFrag_size;
extern const uint32_t testTriangleFrag_data[];

enum R_GameError
R_TriangleTestInitialize (
    struct R_TriangleTest_Context*                 pContext,
    const struct R_Game_PipelineContextCreateInfo* pCreateInfo)
{
    R_RENDER_ASSERT (pContext);
    R_RENDER_ASSERT (pCreateInfo);

    memset (pContext, 0, sizeof (*pContext));
    enum R_GameError result = R_Game_NewPipelineContext (&pContext->pipelineContext, pCreateInfo);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Failed to create pipeline context: %d", result);
        return result;
    }

    struct R_CVulkan_Device* pDevice = R_Game_PipelineContextGetDevice (&pContext->pipelineContext);
    if (!pDevice)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Failed to get device from pipeline context");
        R_Game_PipelineContextDelete (&pContext->pipelineContext);
        return R_GAME_ERROR_NOT_INITIALIZED;
    }

    if (testTriangleVert_size == 0 || !testTriangleVert_data)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Vertex shader data is invalid");
        R_Game_PipelineContextDelete (&pContext->pipelineContext);
        return R_GAME_ERROR_INVALID_ARGUMENT;
    }

    if (testTriangleFrag_size == 0 || !testTriangleFrag_data)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Fragment shader data is invalid");
        R_Game_PipelineContextDelete (&pContext->pipelineContext);
        return R_GAME_ERROR_INVALID_ARGUMENT;
    }

    struct R_CVulkan_ShaderModule vertexShader;
    struct R_CVulkan_ShaderModule fragmentShader;
    memset (&vertexShader, 0, sizeof (vertexShader));
    memset (&fragmentShader, 0, sizeof (fragmentShader));

    enum R_CVulkanError vertResult
        = R_CVulkan_NewShaderModule (&vertexShader, pDevice, testTriangleVert_data, testTriangleVert_size);
    if (vertResult != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Failed to create vertex shader module: %d", vertResult);
        R_Game_PipelineContextDelete (&pContext->pipelineContext);
        return R_GAME_ERROR_INITIALIZATION_FAILED;
    }

    enum R_CVulkanError fragResult
        = R_CVulkan_NewShaderModule (&fragmentShader, pDevice, testTriangleFrag_data, testTriangleFrag_size);
    if (fragResult != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Failed to create fragment shader module: %d", fragResult);
        R_CVulkan_DeleteShaderModule (&vertexShader);
        R_Game_PipelineContextDelete (&pContext->pipelineContext);
        return R_GAME_ERROR_INITIALIZATION_FAILED;
    }
    struct R_CVulkan_PipelineLayoutCreateInfo layoutCreateInfo = {0};
    layoutCreateInfo.pDevice = pDevice;

    R_CSTL_LOG_INFO ("TriangleTest_Initialize: Creating pipeline layout with device %p", (void*)pDevice);
    enum R_CVulkanError layoutResult
        = R_CVulkan_NewPipelineLayout (&pContext->pipelineLayout, &layoutCreateInfo);
    if (layoutResult != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Failed to create pipeline layout: %d", layoutResult);
        R_CVulkan_DeleteShaderModule (&fragmentShader);
        R_CVulkan_DeleteShaderModule (&vertexShader);
        R_Game_PipelineContextDelete (&pContext->pipelineContext);
        return R_GAME_ERROR_INITIALIZATION_FAILED;
    }

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = R_CVulkan_ShaderModuleGetHandle (&vertexShader);
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = R_CVulkan_ShaderModuleGetHandle (&fragmentShader);
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDynamicState                   dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;

    struct R_CVulkan_GraphicsPipelineCreateInfo pipelineCreateInfo = {0};
    pipelineCreateInfo.pDevice = pDevice;
    pipelineCreateInfo.pipelineLayout = R_CVulkan_PipelineLayoutGetHandle (pContext->pipelineLayout);
    pipelineCreateInfo.pRenderPass = VK_NULL_HANDLE;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pVertexInputInfo = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyInfo = &inputAssembly;
    pipelineCreateInfo.pViewportInfo = &viewportState;
    pipelineCreateInfo.pRasterizationInfo = &rasterizer;
    pipelineCreateInfo.pMultisampleInfo = &multisampling;
    pipelineCreateInfo.pDepthStencilInfo = NULL;
    pipelineCreateInfo.pColorBlendInfo = &colorBlending;
    pipelineCreateInfo.pDynamicStateInfo = &dynamicState;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.colorAttachmentCount = 1;
    pipelineCreateInfo.pColorAttachmentFormats = &colorFormat;
    pipelineCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipelineCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    pContext->graphicsPipeline
        = (struct R_CVulkan_Pipeline*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_Pipeline));
    if (!pContext->graphicsPipeline)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Failed to allocate graphics pipeline");
        R_CVulkan_DeletePipelineLayout (pContext->pipelineLayout);
        R_CSTL_HeapUnregisterAllocation (pContext, pContext->pipelineLayout);
        R_CSTL_HeapFree (pContext->pipelineLayout);
        pContext->pipelineLayout = NULL;
        R_CVulkan_DeleteShaderModule (&fragmentShader);
        R_CVulkan_DeleteShaderModule (&vertexShader);
        R_Game_PipelineContextDelete (&pContext->pipelineContext);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pContext->graphicsPipeline, 0, sizeof (struct R_CVulkan_Pipeline));
    R_CSTL_HeapRegisterAllocation (
        pContext,
        pContext->graphicsPipeline,
        sizeof (struct R_CVulkan_Pipeline),
        R_CSTL_HEAP_NAME (struct R_CVulkan_Pipeline));

    enum R_CVulkanError pipelineResult
        = R_CVulkan_NewDynamicGraphicsPipeline (pContext->graphicsPipeline, &pipelineCreateInfo);
    if (pipelineResult != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_Initialize: Failed to create graphics pipeline: %d", pipelineResult);
        R_CSTL_HeapUnregisterAllocation (pContext, pContext->graphicsPipeline);
        R_CSTL_HeapFree (pContext->graphicsPipeline);
        pContext->graphicsPipeline = NULL;
        R_CVulkan_DeletePipelineLayout (pContext->pipelineLayout);
        R_CSTL_HeapUnregisterAllocation (pContext, pContext->pipelineLayout);
        R_CSTL_HeapFree (pContext->pipelineLayout);
        pContext->pipelineLayout = NULL;
        R_CVulkan_DeleteShaderModule (&fragmentShader);
        R_CVulkan_DeleteShaderModule (&vertexShader);
        R_Game_PipelineContextDelete (&pContext->pipelineContext);
        return R_GAME_ERROR_INITIALIZATION_FAILED;
    }

    R_CVulkan_DeleteShaderModule (&fragmentShader);
    R_CVulkan_DeleteShaderModule (&vertexShader);

    R_CSTL_LOG_INFO ("TriangleTest_Initialize: Initialization successful");
    return R_GAME_OK;
}

enum R_GameError
R_TriangleTestRegisterWithRenderer (
    struct R_TriangleTest_Context*  pContext,
    struct R_GameRendererSubsystem* pSubsystem)
{
    R_RENDER_ASSERT (pContext);
    R_RENDER_ASSERT (pSubsystem);

    if (!pContext || !pSubsystem)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_RegisterWithRenderer: Invalid parameters");
        return R_GAME_ERROR_INVALID_ARGUMENT;
    }

    pContext->pRendererSubsystem = pSubsystem;

    enum R_GameError result = R_GameRenderer_AddLayer (
        pSubsystem,
        "TriangleTest",
        0,
        R_GAME_RENDERER_LAYER_FLAG_ENABLED,
        pContext);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_RegisterWithRenderer: Failed to add layer: %d", result);
        pContext->pRendererSubsystem = NULL;
        return result;
    }

    result = R_GameRenderer_SetLayerRenderCallback (
        pSubsystem,
        pContext->layerIndex,
        R_TriangleTestRenderCallback);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_RegisterWithRenderer: Failed to set render callback: %d", result);
        R_GameRenderer_RemoveLayer (pSubsystem, pContext->layerIndex);
        pContext->pRendererSubsystem = NULL;
        return result;
    }

    result = R_GameRenderer_SetLayerBeforePassCallback (
        pSubsystem,
        pContext->layerIndex,
        R_TriangleTestBeforePassCallback);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR (
            "TriangleTest_RegisterWithRenderer: Failed to set before pass callback: %d",
            result);
        R_GameRenderer_RemoveLayer (pSubsystem, pContext->layerIndex);
        pContext->pRendererSubsystem = NULL;
        return result;
    }

    result = R_GameRenderer_SetLayerAfterPassCallback (
        pSubsystem,
        pContext->layerIndex,
        R_TriangleTestAfterPassCallback);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTest_RegisterWithRenderer: Failed to set after pass callback: %d", result);
        R_GameRenderer_RemoveLayer (pSubsystem, pContext->layerIndex);
        pContext->pRendererSubsystem = NULL;
        return result;
    }

    R_CSTL_LOG_INFO ("TriangleTest_RegisterWithRenderer: Registration successful");
    return R_GAME_OK;
}

enum R_GameError
R_TriangleTestRenderFrame (struct R_TriangleTest_Context* pContext)
{
    R_RENDER_ASSERT (pContext);

    if (!pContext)
    {
        R_CSTL_LOG_ERROR ("TriangleTestRenderFrame: Invalid context");
        return R_GAME_ERROR_INVALID_ARGUMENT;
    }

    if (!pContext->pRendererSubsystem)
    {
        R_CSTL_LOG_ERROR ("TriangleTestRenderFrame: Renderer subsystem not set");
        return R_GAME_ERROR_NOT_INITIALIZED;
    }

    enum R_GameError result = R_GameRenderer_BeginFrame (pContext->pRendererSubsystem);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTestRenderFrame: Failed to begin frame: %d", result);
        return result;
    }

    result = R_GameRenderer_RenderFrame (pContext->pRendererSubsystem);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTestRenderFrame: Failed to render frame: %d", result);
        return result;
    }

    result = R_GameRenderer_EndFrame (pContext->pRendererSubsystem);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTestRenderFrame: Failed to end frame: %d", result);
        return result;
    }

    result = R_GameRenderer_WaitForFrame (pContext->pRendererSubsystem);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTestRenderFrame: Failed to wait for frame: %d", result);
        return result;
    }

    return R_GAME_OK;
}

R_RENDER_API void
R_TriangleTestBeforePassCallback (void* pUserData, const void* pResource, const size_t resourceSize)
{
    R_CSTL_LOG_INFO ("TriangleTestBeforePassCallback: Called");
    R_RENDER_ASSERT (pUserData);
    R_RENDER_ASSERT (pResource);

    if (!pUserData || !pResource)
    {
        R_CSTL_LOG_ERROR ("TriangleTestBeforePassCallback: Invalid parameters");
        return;
    }
    struct R_TriangleTest_Context*  pContext = (struct R_TriangleTest_Context*)pUserData;
    struct R_CVulkan_CommandBuffer* pCommandBuffer = (struct R_CVulkan_CommandBuffer*)pResource;

    struct R_Game_PipelineContext* pPipelineContext = &pContext->pipelineContext;
    uint32_t                       currentFrameIndex = pContext->pipelineContext.currentFrameIndex;

    VkClearValue clearValue = {0};
    clearValue.color.float32[0] = 0.0f;
    clearValue.color.float32[1] = 0.0f;
    clearValue.color.float32[2] = 1.0f;
    clearValue.color.float32[3] = 1.0f;

    VkRect2D renderArea = {0};
    renderArea.offset.x = 0;
    renderArea.offset.y = 0;
    renderArea.extent = R_CVulkan_SwapchainGetExtent (&pPipelineContext->swapchain);

    struct R_CVulkan_RenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.renderPass = R_CVulkan_RenderPassGetHandle (&pPipelineContext->renderPass);
    renderPassInfo.framebuffer
        = R_CVulkan_FramebufferGetHandle (&pPipelineContext->pFramebuffers[currentFrameIndex]);
    renderPassInfo.pRenderArea = &renderArea;
    renderPassInfo.contents = VK_SUBPASS_CONTENTS_INLINE;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    enum R_CVulkanError err = R_CVulkan_CommandBufferBeginRenderPass (pCommandBuffer, &renderPassInfo);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("TriangleTestBeforePassCallback: Failed to begin render pass: %d", err);
    }
    else
    {
        R_CSTL_LOG_DEBUG ("TriangleTestBeforePassCallback: Render pass begun successfully");
    }
}

R_RENDER_API void
R_TriangleTestRenderCallback (void* pUserData, const void* pResource, const size_t resourceSize)
{
    R_CSTL_LOG_INFO ("TriangleTestRenderCallback: Called");
    R_RENDER_ASSERT (pUserData);
    R_RENDER_ASSERT (pResource);

    if (!pUserData || !pResource)
    {
        R_CSTL_LOG_ERROR ("TriangleTestRenderCallback: Invalid parameters");
        return;
    }

    struct R_TriangleTest_Context*  pContext = (struct R_TriangleTest_Context*)pUserData;
    struct R_CVulkan_CommandBuffer* pCommandBuffer = (struct R_CVulkan_CommandBuffer*)pResource;

    if (!pContext->graphicsPipeline)
    {
        R_CSTL_LOG_ERROR ("TriangleTestRenderCallback: Graphics pipeline not initialized");
        return;
    }

    VkViewport viewport = {0};
    viewport.width = 800.0f;
    viewport.height = 600.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.extent.width = 800;
    scissor.extent.height = 600;

    R_CSTL_LOG_DEBUG ("TriangleTestRenderCallback: Setting viewport and scissor, binding pipeline, drawing");
    R_CVulkan_CommandBufferSetViewport (pCommandBuffer, 0, 1, &viewport);
    R_CVulkan_CommandBufferSetScissor (pCommandBuffer, 0, 1, &scissor);
    R_CVulkan_CommandBufferBindPipeline (
        pCommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        R_CVulkan_PipelineGetHandle (pContext->graphicsPipeline));
    R_CVulkan_CommandBufferDraw (pCommandBuffer, 3, 1, 0, 0);
    R_CSTL_LOG_DEBUG ("TriangleTestRenderCallback: Draw command issued");
}

R_RENDER_API void
R_TriangleTestAfterPassCallback (void* pUserData, const void* pResource, const size_t resourceSize)
{
    R_CSTL_LOG_INFO ("TriangleTestAfterPassCallback: Called");
    R_RENDER_ASSERT (pUserData);
    R_RENDER_ASSERT (pResource);

    if (!pUserData || !pResource)
    {
        R_CSTL_LOG_ERROR ("TriangleTestAfterPassCallback: Invalid parameters");
        return;
    }

    struct R_CVulkan_CommandBuffer* pCommandBuffer = (struct R_CVulkan_CommandBuffer*)pResource;
    R_CSTL_LOG_DEBUG ("TriangleTestAfterPassCallback: Ending render pass");
    R_CVulkan_CommandBufferEndRenderPass (pCommandBuffer);
    R_CSTL_LOG_DEBUG ("TriangleTestAfterPassCallback: Render pass ended");
}

R_RENDER_API void
R_TriangleTestCleanup (struct R_TriangleTest_Context* pContext)
{
    if (!pContext)
    {
        return;
    }

    if (pContext->pRendererSubsystem)
    {
        R_GameRenderer_RemoveLayer (pContext->pRendererSubsystem, pContext->layerIndex);
        pContext->pRendererSubsystem = NULL;
    }

    if (pContext->graphicsPipeline)
    {
        R_CVulkan_DeletePipeline (pContext->graphicsPipeline);
        R_CSTL_HeapUnregisterAllocation (pContext, pContext->graphicsPipeline);
        R_CSTL_HeapFree (pContext->graphicsPipeline);
        pContext->graphicsPipeline = NULL;
    }

    if (pContext->pipelineLayout)
    {
        R_CVulkan_DeletePipelineLayout (pContext->pipelineLayout);
        R_CSTL_HeapUnregisterAllocation (pContext, pContext->pipelineLayout);
        R_CSTL_HeapFree (pContext->pipelineLayout);
        pContext->pipelineLayout = NULL;
    }

    R_Game_PipelineContextDelete (&pContext->pipelineContext);
    R_CSTL_LOG_INFO ("TriangleTest_Cleanup: Cleanup complete");
}
