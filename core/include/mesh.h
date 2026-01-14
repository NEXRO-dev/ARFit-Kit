/**
 * @file mesh.h
 * @brief 3D Mesh data structures and utilities
 */

#pragma once

#include "types.h"
#include <memory>
#include <vector>

namespace arfit {

/**
 * @brief Vertex data for rendering
 */
struct Vertex {
  Point3D position;
  Point3D normal;
  Point2D texCoord;
  Point3D tangent;
  Point3D bitangent;
};

/**
 * @brief Triangle face
 */
struct Face {
  uint32_t indices[3];
};

/**
 * @brief 3D Mesh class
 */
class Mesh {
public:
  Mesh();
  ~Mesh();

  // Vertex data
  void setVertices(std::vector<Vertex> vertices);
  const std::vector<Vertex> &getVertices() const;
  std::vector<Vertex> &getVerticesMutable();

  // Face data
  void setFaces(std::vector<Face> faces);
  const std::vector<Face> &getFaces() const;

  // Utility methods
  void calculateNormals();
  void calculateTangents();

  /**
   * @brief Create a basic quad mesh (for testing)
   */
  static std::shared_ptr<Mesh> createQuad(float width, float height);

  /**
   * @brief Create a basic T-shirt template mesh
   */
  static std::shared_ptr<Mesh> createTShirtTemplate();

  /**
   * @brief Create mesh from garment type template
   */
  static std::shared_ptr<Mesh> createFromType(GarmentType type);

  // Bounding box
  struct BoundingBox {
    Point3D min;
    Point3D max;
  };

  BoundingBox getBoundingBox() const;

  // Vertex count
  size_t getVertexCount() const;
  size_t getFaceCount() const;

  // GPU buffer management
  void uploadToGPU();
  void releaseGPU();
  bool isOnGPU() const;
  uint32_t getVertexBufferId() const;
  uint32_t getIndexBufferId() const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace arfit
