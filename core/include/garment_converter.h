/**
 * @file garment_converter.h
 * @brief 2D to 3D garment conversion module
 */

#pragma once

#include "mesh.h"
#include "texture.h"
#include "types.h"
#include <memory>
#include <vector>

namespace arfit {

/**
 * @brief Configuration for garment conversion
 */
struct GarmentConverterConfig {
  bool useServerProcessing = true; // Use hybrid server-side processing
  std::string serverEndpoint = "";
  int maxTextureSize = 2048;
  float meshResolution = 0.5f; // 0.0 - 1.0
  bool generateNormalMap = true;
  bool generateDisplacementMap = false;
};

/**
 * @brief Garment representation
 */
class Garment {
public:
  Garment();
  ~Garment();

  GarmentType getType() const;
  void setType(GarmentType type);

  std::shared_ptr<Mesh> getMesh() const;
  void setMesh(std::shared_ptr<Mesh> mesh);

  std::shared_ptr<Texture> getTexture() const;
  void setTexture(std::shared_ptr<Texture> texture);

  std::shared_ptr<Texture> getNormalMap() const;
  void setNormalMap(std::shared_ptr<Texture> normalMap);

  // UV mapping data
  const std::vector<Point2D> &getUVCoords() const;
  void setUVCoords(std::vector<Point2D> uvCoords);

  // Bone rigging data for animation
  struct BoneWeight {
    int boneIndex;
    float weight;
  };

  const std::vector<std::vector<BoneWeight>> &getBoneWeights() const;
  void setBoneWeights(std::vector<std::vector<BoneWeight>> weights);

  // Physics simulation data
  struct ClothParticle {
    Point3D position;
    Point3D velocity;
    float mass;
    bool isPinned;
  };

  const std::vector<ClothParticle> &getClothParticles() const;
  void setClothParticles(std::vector<ClothParticle> particles);

  // Constraints for cloth simulation
  struct SpringConstraint {
    int particleA;
    int particleB;
    float restLength;
    float stiffness;
  };

  const std::vector<SpringConstraint> &getConstraints() const;
  void setConstraints(std::vector<SpringConstraint> constraints);

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 2D image segmentation result
 */
struct SegmentationResult {
  ImageData mask;       // Binary mask of garment
  ImageData frontImage; // Front view of garment
  GarmentType detectedType;
  float confidence;
};

/**
 * @brief Garment converter for 2D to 3D conversion
 */
class GarmentConverter {
public:
  GarmentConverter();
  ~GarmentConverter();

  // Prevent copying
  GarmentConverter(const GarmentConverter &) = delete;
  GarmentConverter &operator=(const GarmentConverter &) = delete;

  /**
   * @brief Initialize the converter
   * @param config Converter configuration
   * @return Result indicating success or failure
   */
  Result<void>
  initialize(const GarmentConverterConfig &config = GarmentConverterConfig{});

  /**
   * @brief Detect and segment garment from image
   * @param image Input image
   * @return Segmentation result
   */
  Result<SegmentationResult> segmentGarment(const ImageData &image);

  /**
   * @brief Detect garment type from image
   * @param image Input image
   * @return Detected garment type
   */
  Result<GarmentType> detectGarmentType(const ImageData &image);

  /**
   * @brief Convert 2D garment image to 3D model
   * @param image Garment image
   * @param type Garment type (auto-detected if UNKNOWN)
   * @return Converted garment
   */
  Result<std::shared_ptr<Garment>>
  convert(const ImageData &image, GarmentType type = GarmentType::UNKNOWN);

  /**
   * @brief Convert using server-side processing (hybrid approach)
   * @param imageUrl URL of the garment image
   * @return Converted garment
   */
  Result<std::shared_ptr<Garment>>
  convertFromServer(const std::string &imageUrl);

  /**
   * @brief Generate UV mapping for garment mesh
   * @param garment Garment to generate UV for
   */
  Result<void> generateUVMapping(std::shared_ptr<Garment> garment);

  /**
   * @brief Setup cloth simulation data for garment
   * @param garment Garment to setup for simulation
   */
  Result<void> setupClothSimulation(std::shared_ptr<Garment> garment);

  /**
   * @brief Check if converter is initialized
   */
  bool isInitialized() const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace arfit
