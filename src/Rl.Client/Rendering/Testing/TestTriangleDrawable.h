#include "Rl.Base/IGameDrawable.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Client/Rendering/IRenderTarget.h"
#include "Rl.Client/Rendering/RenderTarget.h"
#include <memory>

#include <vulkan/vulkan.hpp>

namespace rl
{

class TestTriangleDrawable : public IGameDrawable
{
                VkPipelineLayout pipelineLayout   = VK_NULL_HANDLE;
                VkPipeline       graphicsPipeline = VK_NULL_HANDLE;
                GameShaderModule vertShaderModule;
                GameShaderModule fragShaderModule;

                RenderTarget renderTarget;

        public:
                TestTriangleDrawable() noexcept;
                void setup(GameDeviceInstance& instance) override;
                void draw(GameDeviceInstance& instance) override;
                void destroy(GameDeviceInstance& instance) override;

                const IRenderTarget& getRenderTarget() const
                {
                        return renderTarget;
                }
};
template <> struct IGameDrawableID<TestTriangleDrawable>
{
                inline static TestTriangleDrawable instance{};
};

} // namespace rl
