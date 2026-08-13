#include "Rl.Base/GameMatrix.h"
#include "Rl.Base/IGameDrawable.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanAllocator.h"

#include "Rl.Client/Rendering/IRenderTarget.h"
#include "Rl.Client/Rendering/RenderTarget.h"

#include <vulkan/vulkan.hpp>

namespace rl
{

class TestTriangle3dDrawable : public IGameDrawable
{
                VkPipelineLayout pipelineLayout   = VK_NULL_HANDLE;
                VkPipeline       graphicsPipeline = VK_NULL_HANDLE;

                GameVulkanMemoryAllocator memoryAllocator;
                GameVulkanBuffer          vertexBuffer;
                GameShaderModule          vertShaderModule;
                GameShaderModule          fragShaderModule;
                RenderTarget              renderTarget;

        public:
                TestTriangle3dDrawable() noexcept;
                void setup(GameDeviceInstance& device) override;
                void draw(GameDeviceInstance& device) override;
                void destroy(GameDeviceInstance& device) override;

                const IRenderTarget& getRenderTarget() const
                {
                        return renderTarget;
                }
};

template <> struct IGameDrawableID<TestTriangle3dDrawable>
{
                inline static TestTriangle3dDrawable instance{};
};

} // namespace rl
