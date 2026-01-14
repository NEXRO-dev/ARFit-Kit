/**
 * @file render_pipeline.h
 * @brief Multi-pass rendering pipeline for photorealistic AR compositing
 */

#pragma once

#include "gpu_backend.h"
#include "types.h"
#include <memory>
#include <vector>

namespace arfit {

/**
 * Render pass types for the multi-pass pipeline
 */
enum class RenderPass {
  BODY_DEPTH,   // Pass 1: Render body mesh to depth buffer
  SHADOW_MAP,   // Pass 2: Generate shadow map from light's perspective
  GARMENT_MAIN, // Pass 3: Render garments with occlusion
  LIGHTING,     // Pass 4: Apply PBR lighting
  POST_PROCESS, // Pass 5: Tone mapping, color grading
  COMPOSITE     // Pass 6: Final composite with camera background
};

/**
 * Framebuffer target for render passes
 */
struct RenderTarget {
  int width = 0;
  int height = 0;
  std::shared_ptr<IGPUBuffer> colorBuffer;
  std::shared_ptr<IGPUBuffer> depthBuffer;
  std::shared_ptr<IGPUBuffer> normalBuffer; // For SSAO/lighting
};

/**
 * Light source for scene illumination
 */
struct Light {
  enum class Type { DIRECTIONAL, POINT, SPOT };

  Type type = Type::DIRECTIONAL;
  Point3D position;
  Point3D direction;
  Point3D color = {1.0f, 1.0f, 1.0f};
  float intensity = 1.0f;
  float range = 10.0f;     // For point/spot lights
  float spotAngle = 45.0f; // For spot lights
};

/**
 * Environment lighting estimated from camera
 */
struct EnvironmentLight {
  // Spherical Harmonics coefficients (L0, L1, L2)
  float shCoefficients[9][3];

  // Dominant light direction (estimated)
  Point3D mainLightDirection;
  Point3D mainLightColor;
  float mainLightIntensity;

  // Ambient
  Point3D ambientColor;
  float ambientIntensity;
};

/**
 * Camera/View uniforms
 */
struct ViewUniforms {
  Matrix4x4 viewMatrix;
  Matrix4x4 projectionMatrix;
  Matrix4x4 viewProjectionMatrix;
  Point3D cameraPosition;
  Point3D cameraDirection;
  float nearPlane;
  float farPlane;
  int viewportWidth;
  int viewportHeight;
};

/**
 * Material properties for fabric rendering
 */
struct FabricMaterial {
  enum class Type {
    COTTON,
    SILK,
    DENIM,
    LEATHER,
    VELVET,
    WOOL,
    POLYESTER,
    CUSTOM
  };

  Type type = Type::COTTON;

  // PBR properties
  float roughness = 0.8f;
  float metallic = 0.0f;
  float anisotropy = 0.0f;

  // Fabric-specific
  float sheenIntensity = 0.2f;
  Point3D sheenColor = {1.0f, 1.0f, 1.0f};
  float subsurfaceScattering = 0.1f;
  Point3D subsurfaceColor = {1.0f, 0.9f, 0.8f};

  // Thread direction for anisotropic reflection
  float threadAngle = 0.0f;
};

/**
 * Renderable garment with full material info
 */
struct RenderableGarment {
  std::string id;
  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<Texture> albedoMap;
  std::shared_ptr<Texture> normalMap;
  std::shared_ptr<Texture> roughnessMap;
  FabricMaterial material;
  Matrix4x4 transform;
  bool visible = true;
  bool castsShadow = true;
  bool receivesShadow = true;
};

/**
 * Multi-pass rendering pipeline
 */
class RenderPipeline {
public:
  RenderPipeline();
  ~RenderPipeline();

  /**
   * Initialize the pipeline with GPU context
   */
  bool initialize(std::shared_ptr<IGPUContext> gpuContext, int width,
                  int height);

  /**
   * Resize all render targets
   */
  void resize(int width, int height);

  /**
   * Set camera/view parameters
   */
  void setViewUniforms(const ViewUniforms &uniforms);

  /**
   * Set environment lighting (estimated from camera)
   */
  void setEnvironmentLight(const EnvironmentLight &light);

  /**
   * Add a light source
   */
  void addLight(const Light &light);
  void clearLights();

  /**
   * Set body mesh for occlusion
   */
  void setBodyMesh(std::shared_ptr<Mesh> bodyMesh, const Matrix4x4 &transform);

  /**
   * Add garment to render queue
   */
  void addGarment(const RenderableGarment &garment);
  void clearGarments();

  /**
   * Execute the full rendering pipeline
   * Returns the final composited frame
   */
  ImageData render(const ImageData &cameraBackground);

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;

  // Individual render passes
  void executeBodyDepthPass();
  void executeShadowMapPass();
  void executeGarmentPass();
  void executeLightingPass();
  void executePostProcessPass();
  void executeCompositePass(const ImageData &background);
};

/**
 * Estimate environment lighting from camera frame
 * Uses spherical harmonics to represent lighting
 */
EnvironmentLight estimateEnvironmentLight(const ImageData &cameraFrame);

} // namespace arfit
