/**
 * @file ar_renderer.cpp
 * @brief ARレンダリングエンジンの実装
 */

#include "ar_renderer.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
#include <cmath>

namespace arfit {

/**
 * @brief 描画対象のオブジェクト
 */
struct RenderObject {
  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<Texture> texture;
  Transform transform;
  bool visible;
};

class ARRenderer::Impl {
public:
  RenderConfig config;
  bool initialized = false;

  CameraFrame currentFrame;
  std::vector<RenderObject> garments;
  
  std::vector<uint8_t> framebuffer;
  std::vector<float> depthBuffer;
  int width = 0;
  int height = 0;

  Impl() {}

  void resize(int w, int h) {
    width = w;
    height = h;
    framebuffer.resize(width * height * 4);
    depthBuffer.resize(width * height);
  }

  Point2D project(const Point3D& p, float& outZ) {
      // z座標で除算して透視投影を実現
      outZ = p.z + 2.5f; 
      if (outZ < 0.1f) outZ = 0.1f;
      
      float fov = 1.2f; 
      float x = (p.x * fov / outZ) * (height / (float)width);
      float y = (p.y * fov / outZ);
      
      return {
          (x + 1.0f) * 0.5f * width,
          (1.0f - y) * 0.5f * height
      };
  }

  /**
   * 重心座標(Barycentric coordinates)の計算
   */
  bool barycentric(Point2D a, Point2D b, Point2D c, Point2D p, float &u, float &v, float &w) {
      Point2D v0 = {b.x - a.x, b.y - a.y};
      Point2D v1 = {c.x - a.x, c.y - a.y};
      Point2D v2 = {p.x - a.x, p.y - a.y};
      float d00 = v0.x * v0.x + v0.y * v0.y;
      float d01 = v0.x * v1.x + v0.y * v1.y;
      float d11 = v1.x * v1.x + v1.y * v1.y;
      float d20 = v2.x * v0.x + v2.y * v0.y;
      float d21 = v2.x * v1.x + v2.y * v1.y;
      float denom = d00 * d11 - d01 * d01;
      if (std::abs(denom) < 1e-6) return false;
      v = (d11 * d20 - d01 * d21) / denom;
      w = (d00 * d21 - d01 * d20) / denom;
      u = 1.0f - v - w;
      return (v >= 0) && (w >= 0) && (u >= 0);
  }

  void drawGarments() {
    // 深度バッファを初期化 (遠くの値をセット)
    std::fill(depthBuffer.begin(), depthBuffer.end(), 1000.0f);

    for (const auto &obj : garments) {
      if (!obj.visible || !obj.mesh) continue;

      const auto& vertices = obj.mesh->getVertices();
      const auto& faces = obj.mesh->getFaces();
      
      for (const auto& face : faces) {
          const auto& v0 = vertices[face.indices[0]];
          const auto& v1 = vertices[face.indices[1]];
          const auto& v2 = vertices[face.indices[2]];

          float z0, z1, z2;
          Point2D p0 = project(v0.position, z0);
          Point2D p1 = project(v1.position, z1);
          Point2D p2 = project(v2.position, z2);
          
          // 面の法線の平均を使ったランバート反射ライティング
          Point3D avgNormal = {
              (v0.normal.x + v1.normal.x + v2.normal.x) / 3.0f,
              (v0.normal.y + v1.normal.y + v2.normal.y) / 3.0f,
              (v0.normal.z + v1.normal.z + v2.normal.z) / 3.0f
          };
          Point3D lightDir = {0.2f, 0.5f, 1.0f}; 
          float lightLen = std::sqrt(lightDir.x*lightDir.x + lightDir.y*lightDir.y + lightDir.z*lightDir.z);
          lightDir = lightDir * (1.0f / lightLen);
          float lightIntensity = std::max(0.3f, avgNormal.x*lightDir.x + avgNormal.y*lightDir.y + avgNormal.z*lightDir.z);

          // バウンディングボックス
          int minX = std::max(0, (int)std::floor(std::min({p0.x, p1.x, p2.x})));
          int maxX = std::min(width - 1, (int)std::ceil(std::max({p0.x, p1.x, p2.x})));
          int minY = std::max(0, (int)std::floor(std::min({p0.y, p1.y, p2.y})));
          int maxY = std::min(height - 1, (int)std::ceil(std::max({p0.y, p1.y, p2.y})));
          
          for (int y = minY; y <= maxY; ++y) {
              for (int x = minX; x <= maxX; ++x) {
                  float bu, bv, bw;
                  if (barycentric(p0, p1, p2, {(float)x, (float)y}, bu, bv, bw)) {
                      float z = bu * z0 + bv * z1 + bw * z2;
                      int idx = y * width + x;
                      
                      if (z < depthBuffer[idx]) {
                          depthBuffer[idx] = z;
                          
                          // 重心座標でUV座標を補間
                          float texU = bu * v0.texCoord.x + bv * v1.texCoord.x + bw * v2.texCoord.x;
                          float texV = bu * v0.texCoord.y + bv * v1.texCoord.y + bw * v2.texCoord.y;
                          
                          uint8_t tr, tg, tb, ta;
                          if (obj.texture) {
                              // テクスチャからピクセルをサンプリング
                              obj.texture->sample(texU, texV, tr, tg, tb, ta);
                          } else {
                              tr = 200; tg = 200; tb = 200; ta = 255;
                          }
                          
                          // ライティング適用
                          int px = idx * 4;
                          
                          // アルファブレンディング（テクスチャの透明部分は背景を透過）
                          if (ta > 10) {
                              float alpha = ta / 255.0f;
                              framebuffer[px] = (uint8_t)std::min(255.0f, tr * lightIntensity * alpha + framebuffer[px] * (1.0f - alpha));
                              framebuffer[px+1] = (uint8_t)std::min(255.0f, tg * lightIntensity * alpha + framebuffer[px+1] * (1.0f - alpha));
                              framebuffer[px+2] = (uint8_t)std::min(255.0f, tb * lightIntensity * alpha + framebuffer[px+2] * (1.0f - alpha));
                              framebuffer[px+3] = 255;
                          }
                      }
                  }
              }
          }
      }
    }
  }


  void drawBackground() {
    if (currentFrame.image.pixels.empty()) return;
    if (width != currentFrame.image.width || height != currentFrame.image.height) {
      resize(currentFrame.image.width, currentFrame.image.height);
    }
    std::memcpy(framebuffer.data(), currentFrame.image.pixels.data(), framebuffer.size());
  }
};

ARRenderer::ARRenderer() : pImpl(std::make_unique<Impl>()) {}
ARRenderer::~ARRenderer() = default;

Result<void> ARRenderer::initialize(const RenderConfig &config) {
  pImpl->config = config;
  pImpl->initialized = true;
  return {.error = ErrorCode::SUCCESS};
}

void ARRenderer::setCameraFrame(const CameraFrame &frame) {
  if (!pImpl->initialized) return;
  pImpl->currentFrame = frame;
}

void ARRenderer::addGarment(std::shared_ptr<Garment> garment, const std::vector<Point3D> &positions) {
  RenderObject obj;
  obj.mesh = garment->getMesh();
  obj.texture = garment->getTexture();
  obj.visible = true;
  obj.transform = Transform::identity();
  pImpl->garments.push_back(obj);
  updateGarmentMesh(garment, positions);
}

void ARRenderer::updateGarmentMesh(std::shared_ptr<Garment> garment, const std::vector<Point3D> &positions) {
  for (auto &obj : pImpl->garments) {
    if (obj.mesh == garment->getMesh() && obj.mesh->getVertexCount() == positions.size()) {
      auto& vertices = obj.mesh->getVerticesMutable();
      for (size_t i = 0; i < positions.size(); ++i) vertices[i].position = positions[i];
      obj.mesh->calculateNormals(); // 形状変化に合わせて法線を再計算
      break;
    }
  }
}

void ARRenderer::removeGarment(std::shared_ptr<Garment> garment) {
  auto it = std::remove_if(pImpl->garments.begin(), pImpl->garments.end(),
      [&](const RenderObject &obj) { return obj.mesh == garment->getMesh(); });
  pImpl->garments.erase(it, pImpl->garments.end());
}

Result<ImageData> ARRenderer::render() {
  if (!pImpl->initialized) return {.error = ErrorCode::INITIALIZATION_FAILED};
  pImpl->drawBackground();
  pImpl->drawGarments();
  ImageData result;
  result.width = pImpl->width;
  result.height = pImpl->height;
  result.channels = 4;
  result.pixels = pImpl->framebuffer;
  return {.value = result, .error = ErrorCode::SUCCESS};
}

void ARRenderer::setProjectionMatrix(const Transform &projection) {}
void ARRenderer::setViewMatrix(const Transform &view) {}
bool ARRenderer::isInitialized() const { return pImpl->initialized; }
std::string ARRenderer::getBackendType() const { return "Software"; }

} // namespace arfit
