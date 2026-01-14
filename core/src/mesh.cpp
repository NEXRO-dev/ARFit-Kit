/**
 * @file mesh.cpp
 * @brief 3D Mesh implementation
 */

#include "mesh.h"
#include <cmath>

namespace arfit {

class Mesh::Impl {
public:
  std::vector<Vertex> vertices;
  std::vector<Face> faces;

  bool onGPU = false;
  uint32_t vertexBufferId = 0;
  uint32_t indexBufferId = 0;
};

Mesh::Mesh() : pImpl(std::make_unique<Impl>()) {}
Mesh::~Mesh() = default;

void Mesh::setVertices(std::vector<Vertex> vertices) {
  pImpl->vertices = std::move(vertices);
}

const std::vector<Vertex> &Mesh::getVertices() const { return pImpl->vertices; }

std::vector<Vertex> &Mesh::getVerticesMutable() { return pImpl->vertices; }

void Mesh::setFaces(std::vector<Face> faces) {
  pImpl->faces = std::move(faces);
}

const std::vector<Face> &Mesh::getFaces() const { return pImpl->faces; }

void Mesh::calculateNormals() {
  // Zero all normals
  for (auto &v : pImpl->vertices) {
    v.normal = {0, 0, 0};
  }

  // Accumulate face normals
  for (const auto &face : pImpl->faces) {
    const auto &v0 = pImpl->vertices[face.indices[0]].position;
    const auto &v1 = pImpl->vertices[face.indices[1]].position;
    const auto &v2 = pImpl->vertices[face.indices[2]].position;

    // Calculate face normal
    Point3D e1 = v1 - v0;
    Point3D e2 = v2 - v0;

    Point3D normal = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z,
                      e1.x * e2.y - e1.y * e2.x};

    // Add to vertex normals
    for (int i = 0; i < 3; ++i) {
      pImpl->vertices[face.indices[i]].normal =
          pImpl->vertices[face.indices[i]].normal + normal;
    }
  }

  // Normalize
  for (auto &v : pImpl->vertices) {
    float len = std::sqrt(v.normal.x * v.normal.x + v.normal.y * v.normal.y +
                          v.normal.z * v.normal.z);
    if (len > 0.0001f) {
      v.normal = v.normal * (1.0f / len);
    }
  }
}

void Mesh::calculateTangents() {
  // Calculate tangents for normal mapping
  // Uses Lengyel's method

  for (const auto &face : pImpl->faces) {
    auto &v0 = pImpl->vertices[face.indices[0]];
    auto &v1 = pImpl->vertices[face.indices[1]];
    auto &v2 = pImpl->vertices[face.indices[2]];

    Point3D e1 = v1.position - v0.position;
    Point3D e2 = v2.position - v0.position;

    float du1 = v1.texCoord.x - v0.texCoord.x;
    float dv1 = v1.texCoord.y - v0.texCoord.y;
    float du2 = v2.texCoord.x - v0.texCoord.x;
    float dv2 = v2.texCoord.y - v0.texCoord.y;

    float f = 1.0f / (du1 * dv2 - du2 * dv1 + 0.0001f);

    Point3D tangent = {f * (dv2 * e1.x - dv1 * e2.x),
                       f * (dv2 * e1.y - dv1 * e2.y),
                       f * (dv2 * e1.z - dv1 * e2.z)};

    for (int i = 0; i < 3; ++i) {
      pImpl->vertices[face.indices[i]].tangent =
          pImpl->vertices[face.indices[i]].tangent + tangent;
    }
  }

  // Normalize and orthogonalize
  for (auto &v : pImpl->vertices) {
    float len =
        std::sqrt(v.tangent.x * v.tangent.x + v.tangent.y * v.tangent.y +
                  v.tangent.z * v.tangent.z);
    if (len > 0.0001f) {
      v.tangent = v.tangent * (1.0f / len);
    }

    // Gram-Schmidt orthogonalize
    float dot = v.normal.x * v.tangent.x + v.normal.y * v.tangent.y +
                v.normal.z * v.tangent.z;
    v.tangent = v.tangent - v.normal * dot;

    // Calculate bitangent
    v.bitangent = {v.normal.y * v.tangent.z - v.normal.z * v.tangent.y,
                   v.normal.z * v.tangent.x - v.normal.x * v.tangent.z,
                   v.normal.x * v.tangent.y - v.normal.y * v.tangent.x};
  }
}

std::shared_ptr<Mesh> Mesh::createQuad(float width, float height) {
  auto mesh = std::make_shared<Mesh>();

  float hw = width * 0.5f;
  float hh = height * 0.5f;

  std::vector<Vertex> vertices = {
      {{-hw, -hh, 0}, {0, 0, 1}, {0, 0}, {1, 0, 0}, {0, 1, 0}},
      {{hw, -hh, 0}, {0, 0, 1}, {1, 0}, {1, 0, 0}, {0, 1, 0}},
      {{hw, hh, 0}, {0, 0, 1}, {1, 1}, {1, 0, 0}, {0, 1, 0}},
      {{-hw, hh, 0}, {0, 0, 1}, {0, 1}, {1, 0, 0}, {0, 1, 0}}};

  std::vector<Face> faces = {{{0, 1, 2}}, {{0, 2, 3}}};

  mesh->setVertices(vertices);
  mesh->setFaces(faces);

  return mesh;
}

std::shared_ptr<Mesh> Mesh::createTShirtTemplate() {
  auto mesh = std::make_shared<Mesh>();

  // Create a basic T-shirt shaped mesh
  // This is a simplified version - production would use detailed template

  const int rows = 20;
  const int cols = 15;
  std::vector<Vertex> vertices;

  for (int r = 0; r < rows; ++r) {
    float y = 1.0f - (float(r) / (rows - 1)) * 1.5f; // -0.5 to 1.0

    for (int c = 0; c < cols; ++c) {
      float t = float(c) / (cols - 1);
      float x = (t - 0.5f) * 0.8f; // -0.4 to 0.4

      // Add sleeve width at shoulder level
      if (r >= 2 && r <= 5) {
        float sleeveExtend = 0.3f * (1.0f - std::abs(r - 3.5f) / 2.0f);
        if (t < 0.3f)
          x -= sleeveExtend;
        if (t > 0.7f)
          x += sleeveExtend;
      }

      Vertex v;
      v.position = {x, y, 0.0f};
      v.normal = {0, 0, 1};
      v.texCoord = {t, float(r) / (rows - 1)};
      vertices.push_back(v);
    }
  }

  std::vector<Face> faces;
  for (int r = 0; r < rows - 1; ++r) {
    for (int c = 0; c < cols - 1; ++c) {
      int i = r * cols + c;
      faces.push_back({{static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1),
                        static_cast<uint32_t>(i + cols + 1)}});
      faces.push_back(
          {{static_cast<uint32_t>(i), static_cast<uint32_t>(i + cols + 1),
            static_cast<uint32_t>(i + cols)}});
    }
  }

  mesh->setVertices(vertices);
  mesh->setFaces(faces);
  mesh->calculateNormals();

  return mesh;
}

std::shared_ptr<Mesh> Mesh::createFromType(GarmentType type) {
  switch (type) {
  case GarmentType::TSHIRT:
  case GarmentType::SHIRT:
    return createTShirtTemplate();

  case GarmentType::PANTS:
  case GarmentType::SHORTS:
    // TODO: Create pants template
    return createQuad(0.8f, 1.0f);

  case GarmentType::DRESS:
    // TODO: Create dress template
    return createTShirtTemplate();

  default:
    return createQuad(1.0f, 1.0f);
  }
}

Mesh::BoundingBox Mesh::getBoundingBox() const {
  BoundingBox box;
  box.min = {FLT_MAX, FLT_MAX, FLT_MAX};
  box.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

  for (const auto &v : pImpl->vertices) {
    box.min.x = std::min(box.min.x, v.position.x);
    box.min.y = std::min(box.min.y, v.position.y);
    box.min.z = std::min(box.min.z, v.position.z);
    box.max.x = std::max(box.max.x, v.position.x);
    box.max.y = std::max(box.max.y, v.position.y);
    box.max.z = std::max(box.max.z, v.position.z);
  }

  return box;
}

size_t Mesh::getVertexCount() const { return pImpl->vertices.size(); }

size_t Mesh::getFaceCount() const { return pImpl->faces.size(); }

void Mesh::uploadToGPU() {
  // TODO: Upload to GPU buffer
  pImpl->onGPU = true;
}

void Mesh::releaseGPU() {
  // TODO: Release GPU buffer
  pImpl->onGPU = false;
}

bool Mesh::isOnGPU() const { return pImpl->onGPU; }

uint32_t Mesh::getVertexBufferId() const { return pImpl->vertexBufferId; }

uint32_t Mesh::getIndexBufferId() const { return pImpl->indexBufferId; }

} // namespace arfit
