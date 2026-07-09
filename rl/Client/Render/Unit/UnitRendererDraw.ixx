export module Rl.Client.Render.Unit.UnitRendererDraw;

import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.Render.Unit.UnitRendererShadowMap;
import Rl.Client.State.UnitState;

import <vulkan/vulkan.hpp>;
import <glm/glm.hpp>;

namespace Rl::Client::Render
{

export void UnitUpdateCascadeShadowMaps(const glm::mat4& cameraView,
    const glm::mat4&                          cameraPerspective,
    const glm::vec3&                          sunDirection,
    float                                     cameraAspect,
    float                                     cameraNear,
    float                                     cameraFar,
    uint32_t                                  numCascades,
    UnitCascadeShadowLightingUniforms&        shadowUniforms,
    glm::vec4&                                cascadeSplits,
    uint32_t&                                 cascadeCount);

export void UnitRenderCascadeShadowMap(Providers::UnitStateResource& resource,
    Providers::UnitStateBinding&                              vk,
    Main::MainBinding&                                        context);

export void UnitRender(Providers::UnitStateResource& resource,
    Providers::UnitStateBinding&                     vk,
    Main::MainBinding&                               context);

} // namespace Rl::Client::Render
