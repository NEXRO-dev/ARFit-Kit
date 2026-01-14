/**
 * @file render_pipeline.cpp
 * @brief Multi-pass rendering pipeline implementation
 */

#include "render_pipeline.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace arfit {

class RenderPipeline::Impl {
public:
  std::shared_ptr<IGPUContext> gpu;

  // Render targets
  RenderTarget mainTarget;
  RenderTarget depthTarget;
  RenderTarget shadowTarget;
  RenderTarget lightingTarget;

  // Shaders
  std::shared_ptr<IGPUShader> bodyDepthShader;
  std::shared_ptr<IGPUShader> shadowMapShader;
  std::shared_ptr<IGPUShader> garmentShader;
  std::shared_ptr<IGPUShader> fabricShader;
  std::shared_ptr<IGPUShader> lightingShader;
  std::shared_ptr<IGPUShader> compositeShader;

  // Scene data
  ViewUniforms viewUniforms;
  EnvironmentLight envLight;
  std::vector<Light> lights;
  std::shared_ptr<Mesh> bodyMesh;
  Matrix4x4 bodyTransform;
  std::vector<RenderableGarment> garments;

  // Uniform buffers
  std::shared_ptr<IGPUBuffer> viewUniformBuffer;
  std::shared_ptr<IGPUBuffer> lightUniformBuffer;
  std::shared_ptr<IGPUBuffer> materialUniformBuffer;

  int width = 0;
  int height = 0;
  bool initialized = false;
};

RenderPipeline::RenderPipeline() : pImpl(std::make_unique<Impl>()) {}

RenderPipeline::~RenderPipeline() = default;

bool RenderPipeline::initialize(std::shared_ptr<IGPUContext> gpuContext,
                                int width, int height) {
  if (!gpuContext) {
    std::cerr << "[RenderPipeline] GPU context is null" << std::endl;
    return false;
  }

  pImpl->gpu = gpuContext;
  pImpl->width = width;
  pImpl->height = height;

  // Initialize GPU context
  if (!pImpl->gpu->initialize()) {
    std::cerr << "[RenderPipeline] Failed to initialize GPU context"
              << std::endl;
    return false;
  }

  // Create render targets
  size_t colorBufferSize = width * height * 4 * sizeof(float); // RGBA float
  size_t depthBufferSize = width * height * sizeof(float);

  pImpl->mainTarget.width = width;
  pImpl->mainTarget.height = height;
  pImpl->mainTarget.colorBuffer =
      pImpl->gpu->createBuffer(colorBufferSize, BufferType::STORAGE);
  pImpl->mainTarget.depthBuffer =
      pImpl->gpu->createBuffer(depthBufferSize, BufferType::STORAGE);
  pImpl->mainTarget.normalBuffer =
      pImpl->gpu->createBuffer(colorBufferSize, BufferType::STORAGE);

  // Shadow map (lower resolution)
  int shadowRes = 1024;
  pImpl->shadowTarget.width = shadowRes;
  pImpl->shadowTarget.height = shadowRes;
  pImpl->shadowTarget.depthBuffer = pImpl->gpu->createBuffer(
      shadowRes * shadowRes * sizeof(float), BufferType::STORAGE);

  // Create uniform buffers
  pImpl->viewUniformBuffer =
      pImpl->gpu->createBuffer(sizeof(ViewUniforms), BufferType::UNIFORM);
  pImpl->lightUniformBuffer =
      pImpl->gpu->createBuffer(sizeof(EnvironmentLight), BufferType::UNIFORM);

  // Load shaders (in production, load from files or compile at runtime)
  // For now, we use empty shader objects as placeholders
  pImpl->bodyDepthShader =
      pImpl->gpu->createShader("", ShaderType::VERTEX_FRAGMENT);
  pImpl->shadowMapShader =
      pImpl->gpu->createShader("", ShaderType::VERTEX_FRAGMENT);
  pImpl->garmentShader =
      pImpl->gpu->createShader("", ShaderType::VERTEX_FRAGMENT);
  pImpl->fabricShader =
      pImpl->gpu->createShader("", ShaderType::VERTEX_FRAGMENT);
  pImpl->lightingShader = pImpl->gpu->createShader("", ShaderType::COMPUTE);
  pImpl->compositeShader = pImpl->gpu->createShader("", ShaderType::COMPUTE);

  pImpl->initialized = true;
  std::cout << "[RenderPipeline] Initialized with " << width << "x" << height
            << std::endl;

  return true;
}

void RenderPipeline::resize(int width, int height) {
  if (pImpl->width == width && pImpl->height == height)
    return;

  pImpl->width = width;
  pImpl->height = height;

  // Recreate render targets
  size_t colorBufferSize = width * height * 4 * sizeof(float);
  size_t depthBufferSize = width * height * sizeof(float);

  pImpl->mainTarget.width = width;
  pImpl->mainTarget.height = height;
  pImpl->mainTarget.colorBuffer =
      pImpl->gpu->createBuffer(colorBufferSize, BufferType::STORAGE);
  pImpl->mainTarget.depthBuffer =
      pImpl->gpu->createBuffer(depthBufferSize, BufferType::STORAGE);
  pImpl->mainTarget.normalBuffer =
      pImpl->gpu->createBuffer(colorBufferSize, BufferType::STORAGE);
}

void RenderPipeline::setViewUniforms(const ViewUniforms &uniforms) {
  pImpl->viewUniforms = uniforms;
  if (pImpl->viewUniformBuffer) {
    pImpl->viewUniformBuffer->upload(&uniforms, sizeof(ViewUniforms));
  }
}

void RenderPipeline::setEnvironmentLight(const EnvironmentLight &light) {
  pImpl->envLight = light;
  if (pImpl->lightUniformBuffer) {
    pImpl->lightUniformBuffer->upload(&light, sizeof(EnvironmentLight));
  }
}

void RenderPipeline::addLight(const Light &light) {
  pImpl->lights.push_back(light);
}

void RenderPipeline::clearLights() { pImpl->lights.clear(); }

void RenderPipeline::setBodyMesh(std::shared_ptr<Mesh> bodyMesh,
                                 const Matrix4x4 &transform) {
  pImpl->bodyMesh = bodyMesh;
  pImpl->bodyTransform = transform;
}

void RenderPipeline::addGarment(const RenderableGarment &garment) {
  pImpl->garments.push_back(garment);
}

void RenderPipeline::clearGarments() { pImpl->garments.clear(); }

void RenderPipeline::executeBodyDepthPass() {
  if (!pImpl->bodyMesh)
    return;

  // Render body mesh to depth buffer only
  // This creates occlusion data for arms/hands in front of garments

  pImpl->gpu->beginFrame();

  // Bind depth target
  // Draw body mesh with depth-only shader
  // ... GPU-specific implementation

  pImpl->gpu->endFrame();
}

void RenderPipeline::executeShadowMapPass() {
  // Render scene from light's perspective to create shadow map

  if (pImpl->lights.empty() && pImpl->envLight.mainLightIntensity < 0.1f) {
    return; // No significant light sources
  }

  pImpl->gpu->beginFrame();

  // Set up light-space view matrix
  // Render all shadow-casting objects

  for (const auto &garment : pImpl->garments) {
    if (!garment.castsShadow || !garment.mesh)
      continue;
    // Draw to shadow map
  }

  pImpl->gpu->endFrame();
}

void RenderPipeline::executeGarmentPass() {
  pImpl->gpu->beginFrame();

  for (const auto &garment : pImpl->garments) {
    if (!garment.visible || !garment.mesh)
      continue;

    // Bind material textures
    // Set material uniforms based on FabricMaterial
    // Draw with occlusion testing against body depth

    // In production, dispatch based on material type:
    // - Fabric shader for cloth
    // - Standard PBR for leather/synthetic
  }

  pImpl->gpu->endFrame();
}

void RenderPipeline::executeLightingPass() {
  // Apply deferred lighting / compute lighting contribution

  pImpl->gpu->beginFrame();

  // Dispatch lighting compute shader
  // Input: G-buffer (color, normal, depth)
  // Output: Lit color buffer

  uint32_t workGroupsX = (pImpl->width + 15) / 16;
  uint32_t workGroupsY = (pImpl->height + 15) / 16;

  std::vector<std::shared_ptr<IGPUBuffer>> bindings = {
      pImpl->mainTarget.colorBuffer, pImpl->mainTarget.normalBuffer,
      pImpl->mainTarget.depthBuffer, pImpl->lightUniformBuffer};

  pImpl->gpu->dispatch(pImpl->lightingShader, workGroupsX, workGroupsY, 1,
                       bindings);

  pImpl->gpu->endFrame();
}

void RenderPipeline::executePostProcessPass() {
  // Tone mapping, color grading, bloom, etc.

  pImpl->gpu->beginFrame();

  // For now, just apply simple tone mapping

  pImpl->gpu->endFrame();
}

void RenderPipeline::executeCompositePass(const ImageData &background) {
  // Composite rendered garments over camera background

  pImpl->gpu->beginFrame();

  // Alpha blend garment render over camera frame
  // Apply soft edge blending where garment meets body

  pImpl->gpu->endFrame();
}

ImageData RenderPipeline::render(const ImageData &cameraBackground) {
  if (!pImpl->initialized) {
    std::cerr << "[RenderPipeline] Not initialized" << std::endl;
    return {};
  }

  // Execute multi-pass pipeline
  executeBodyDepthPass();
  executeShadowMapPass();
  executeGarmentPass();
  executeLightingPass();
  executePostProcessPass();
  executeCompositePass(cameraBackground);

  // Read back final frame
  ImageData result;
  result.width = pImpl->width;
  result.height = pImpl->height;
  result.channels = 4;
  result.pixels.resize(pImpl->width * pImpl->height * 4);

  if (pImpl->mainTarget.colorBuffer) {
    pImpl->mainTarget.colorBuffer->download(result.pixels.data(),
                                            result.pixels.size());
  }

  return result;
}

// Environment light estimation from camera frame
EnvironmentLight estimateEnvironmentLight(const ImageData &cameraFrame) {
  EnvironmentLight light;

  if (cameraFrame.pixels.empty()) {
    // Default lighting
    light.mainLightDirection = {0.0f, -1.0f, -0.3f};
    light.mainLightColor = {1.0f, 0.98f, 0.95f};
    light.mainLightIntensity = 1.0f;
    light.ambientColor = {0.2f, 0.22f, 0.25f};
    light.ambientIntensity = 0.3f;
    return light;
  }

  // Sample brightness from different regions
  int w = cameraFrame.width;
  int h = cameraFrame.height;

  float topBrightness = 0, bottomBrightness = 0;
  float leftBrightness = 0, rightBrightness = 0;
  float centerBrightness = 0;
  int samples = 0;

  auto sampleBrightness = [&](int x, int y) -> float {
    if (x < 0 || x >= w || y < 0 || y >= h)
      return 0;
    int idx = (y * w + x) * cameraFrame.channels;
    float r = cameraFrame.pixels[idx] / 255.0f;
    float g = cameraFrame.pixels[idx + 1] / 255.0f;
    float b = cameraFrame.pixels[idx + 2] / 255.0f;
    return 0.299f * r + 0.587f * g + 0.114f * b; // Luma
  };

  // Sample top region
  for (int y = 0; y < h / 4; y += 10) {
    for (int x = 0; x < w; x += 10) {
      topBrightness += sampleBrightness(x, y);
      samples++;
    }
  }
  topBrightness /= std::max(samples, 1);

  // Sample bottom region
  samples = 0;
  for (int y = h * 3 / 4; y < h; y += 10) {
    for (int x = 0; x < w; x += 10) {
      bottomBrightness += sampleBrightness(x, y);
      samples++;
    }
  }
  bottomBrightness /= std::max(samples, 1);

  // Estimate main light direction from brightness gradient
  float yGradient = topBrightness - bottomBrightness;
  light.mainLightDirection = {0.0f, -0.7f - yGradient * 0.3f, -0.3f};

  // Normalize
  float len =
      std::sqrt(light.mainLightDirection.x * light.mainLightDirection.x +
                light.mainLightDirection.y * light.mainLightDirection.y +
                light.mainLightDirection.z * light.mainLightDirection.z);
  if (len > 0) {
    light.mainLightDirection.x /= len;
    light.mainLightDirection.y /= len;
    light.mainLightDirection.z /= len;
  }

  // Set light color/intensity based on overall brightness
  float avgBrightness = (topBrightness + bottomBrightness) / 2.0f;
  light.mainLightIntensity = 0.5f + avgBrightness * 0.5f;
  light.mainLightColor = {1.0f, 0.98f, 0.95f}; // Slightly warm

  light.ambientIntensity = 0.2f + avgBrightness * 0.2f;
  light.ambientColor = {0.3f, 0.32f, 0.35f};

  // Fill SH coefficients (simplified: just use L0 ambient)
  for (int i = 0; i < 9; i++) {
    light.shCoefficients[i][0] = light.ambientColor.x * light.ambientIntensity;
    light.shCoefficients[i][1] = light.ambientColor.y * light.ambientIntensity;
    light.shCoefficients[i][2] = light.ambientColor.z * light.ambientIntensity;
  }

  return light;
}

} // namespace arfit
