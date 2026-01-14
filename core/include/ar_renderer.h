/**
 * @file ar_renderer.h
 * @brief AR rendering module for compositing garments onto camera feed
 */

#pragma once

#include "garment_converter.h"
#include "types.h"
#include <memory>
#include <vector>

namespace arfit {

/**
 * @brief Rendering configuration
 */
struct RenderConfig {
  int outputWidth = 1920;
  int outputHeight = 1080;
  bool enableShadows = true;
  bool enableAntialiasing = true;
  bool enableAmbientOcclusion = false;
  float shadowIntensity = 0.5f;

  // Lighting
  Point3D lightDirection = {0.5f, -1.0f, 0.5f};
  float ambientLight = 0.3f;
};

/**
 * @brief Render target for output
 */
struct RenderTarget {
  uint32_t textureId = 0; // GPU texture ID
  int width = 0;
  int height = 0;
};

/**
 * @brief AR renderer for compositing garments
 */
class ARRenderer {
public:
  ARRenderer();
  ~ARRenderer();

  // Prevent copying
  ARRenderer(const ARRenderer &) = delete;
  ARRenderer &operator=(const ARRenderer &) = delete;

  /**
   * @brief Initialize the renderer
   * @param config Render configuration
   * @return Result indicating success or failure
   */
  Result<void> initialize(const RenderConfig &config = RenderConfig{});

  /**
   * @brief Set the camera feed as background
   * @param frame Camera frame to use as background
   */
  void setCameraFrame(const CameraFrame &frame);

  /**
   * @brief Add a garment to render
   * @param garment Garment to render
   * @param particlePositions Current cloth particle positions
   */
  void addGarment(std::shared_ptr<Garment> garment,
                  const std::vector<Point3D> &particlePositions);

  /**
   * @brief Update garment mesh with new particle positions
   * @param garment Garment to update
   * @param particlePositions New particle positions from physics
   */
  void updateGarmentMesh(std::shared_ptr<Garment> garment,
                         const std::vector<Point3D> &particlePositions);

  /**
   * @brief Remove a garment from rendering
   * @param garment Garment to remove
   */
  void removeGarment(std::shared_ptr<Garment> garment);

  /**
   * @brief Set body occlusion mesh for proper depth ordering
   * @param bodyMesh Body mesh vertices
   * @param transform Body transform
   */
  void setBodyOcclusionMesh(const std::vector<Point3D> &bodyMesh,
                            const Transform &transform);

  /**
   * @brief Render current frame
   * @return Rendered output image
   */
  Result<ImageData> render();

  /**
   * @brief Render to GPU texture (for native integration)
   * @param target Render target
   * @return Result indicating success or failure
   */
  Result<void> renderToTexture(RenderTarget &target);

  /**
   * @brief Set camera projection matrix
   * @param projection 4x4 projection matrix
   */
  void setProjectionMatrix(const Transform &projection);

  /**
   * @brief Set camera view matrix
   * @param view 4x4 view matrix
   */
  void setViewMatrix(const Transform &view);

  /**
   * @brief Update lighting based on environment
   * @param direction Light direction
   * @param intensity Light intensity
   */
  void updateLighting(const Point3D &direction, float intensity);

  /**
   * @brief Check if renderer is initialized
   */
  bool isInitialized() const;

  /**
   * @brief Get render backend type
   * @return Backend name ("Metal", "Vulkan", "WebGPU", "OpenGL")
   */
  std::string getBackendType() const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace arfit
