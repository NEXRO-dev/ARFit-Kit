/**
 * @file garment_converter.cpp
 * @brief 2D衣服画像から3Dメッシュへの変換モジュール実装
 */

#include "garment_converter.h"
#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

namespace arfit {

/**
 * @brief Garmentクラスの内部実装
 */
class Garment::Impl {
public:
    GarmentType type = GarmentType::UNKNOWN;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Texture> normalMap;
    std::vector<Point2D> uvCoords;
    std::vector<std::vector<Garment::BoneWeight>> boneWeights;
    std::vector<Garment::ClothParticle> clothParticles;
    std::vector<Garment::SpringConstraint> constraints;
    bool isLoaded = false;
    ClothMaterial material = ClothMaterial::COTTON;
};

Garment::Garment() : pImpl(std::make_unique<Impl>()) {}
Garment::~Garment() = default;

GarmentType Garment::getType() const { return pImpl->type; }
void Garment::setType(GarmentType type) { pImpl->type = type; }
std::shared_ptr<Mesh> Garment::getMesh() const { return pImpl->mesh; }
void Garment::setMesh(std::shared_ptr<Mesh> mesh) { pImpl->mesh = mesh; }
std::shared_ptr<Texture> Garment::getTexture() const { return pImpl->texture; }
void Garment::setTexture(std::shared_ptr<Texture> texture) { pImpl->texture = texture; }

/**
 * @brief GarmentConverterの内部実装
 */
class GarmentConverter::Impl {
public:
  GarmentConverterConfig config;

  // 衣服の形状テンプレート
  std::shared_ptr<Mesh> tshirtTemplate;
  std::shared_ptr<Mesh> pantsTemplate;

  Impl() { initializeTemplates(); }

  /**
   * 基本的な形状テンプレートを生成
   */
  void initializeTemplates() {
    tshirtTemplate = Mesh::createTShirtTemplate();
  }

  /**
   * 画像から衣服の種類を推定
   */
  GarmentType detectType(const cv::Mat &image) {
    float aspect = static_cast<float>(image.cols) / image.rows;
    if (aspect > 0.8f) return GarmentType::TSHIRT;
    if (aspect < 0.5f) return GarmentType::DRESS;
    return GarmentType::SHIRT;
  }

  /**
   * 衣服の領域を切り出し（セグメンテーション）
   */
  cv::Mat segmentGarment(const cv::Mat &input) {
    cv::Mat mask;
    if (input.channels() == 4) {
      std::vector<cv::Mat> channels;
      cv::split(input, channels);
      mask = channels[3]; // アルファチャンネルをマスクとして使用
    } else {
      mask = cv::Mat::zeros(input.size(), CV_8UC1);
      cv::ellipse(mask, cv::Point(input.cols / 2, input.rows / 2),
                  cv::Size(input.cols / 3, input.rows / 3), 0, 0, 360,
                  cv::Scalar(255), -1);
    }
    return mask;
  }

  /**
   * 2Dのシルエットに合わせて3Dメッシュを変形（フィッティング）
   */
  void fitMeshToSilhouette(std::shared_ptr<Mesh> mesh, const cv::Mat &mask) {
    if (!mesh || mask.empty()) return;

    cv::Rect bounds = cv::boundingRect(mask);
    auto &vertices = const_cast<std::vector<Vertex> &>(mesh->getVertices());
    
    for (auto &v : vertices) {
        // Y座標に基づいてマスクの高さをマッピング
        int yInMask = bounds.y + (1.0f - (v.position.y + 0.5f)) * bounds.height;
        yInMask = std::clamp(yInMask, 0, mask.rows - 1);
        
        cv::Mat row = mask.row(yInMask);
        int left = -1, right = -1;
        for (int x = 0; x < row.cols; ++x) {
            if (row.at<uint8_t>(x) > 128) { if (left == -1) left = x; right = x; }
        }
        
        if (left != -1 && right != -1) {
            float silhouetteWidth = static_cast<float>(right - left) / mask.cols;
            // テンプレートの幅に対してシルエットの幅を適用
            v.position.x *= (silhouetteWidth * 2.5f); 
        }
        
        // 厚みも少し調整
        v.position.z = std::abs(v.position.x) * 0.15f;
    }
  }

  /**
   * メッシュを人体のボーンに紐付ける（リギング）
   */
  std::vector<std::vector<Garment::BoneWeight>> rigToBody(std::shared_ptr<Mesh> mesh, GarmentType type) {
    const auto& vertices = mesh->getVertices();
    std::vector<std::vector<Garment::BoneWeight>> weights(vertices.size());

    for (size_t i = 0; i < vertices.size(); ++i) {
        const auto& pos = vertices[i].position;
        
        if (type == GarmentType::TSHIRT || type == GarmentType::SHIRT) {
            if (pos.y > 0.7f) {
                // 肩・首
                if (pos.x < -0.2f) weights[i].push_back({(int)BodyLandmark::LEFT_SHOULDER, 1.0f});
                else if (pos.x > 0.2f) weights[i].push_back({(int)BodyLandmark::RIGHT_SHOULDER, 1.0f});
                else {
                    weights[i].push_back({(int)BodyLandmark::LEFT_SHOULDER, 0.5f});
                    weights[i].push_back({(int)BodyLandmark::RIGHT_SHOULDER, 0.5f});
                }
            } else if (pos.y < 0.2f) {
                // 腰
                weights[i].push_back({(int)BodyLandmark::LEFT_HIP, 0.5f});
                weights[i].push_back({(int)BodyLandmark::RIGHT_HIP, 0.5f});
            } else {
                // 胴体
                weights[i].push_back({(int)BodyLandmark::LEFT_SHOULDER, 0.25f});
                weights[i].push_back({(int)BodyLandmark::RIGHT_SHOULDER, 0.25f});
                weights[i].push_back({(int)BodyLandmark::LEFT_HIP, 0.25f});
                weights[i].push_back({(int)BodyLandmark::RIGHT_HIP, 0.25f});
            }
        }
    }
    return weights;
  }
};

GarmentConverter::GarmentConverter() : pImpl(std::make_unique<Impl>()) {}
GarmentConverter::~GarmentConverter() = default;

Result<void>
GarmentConverter::initialize(const GarmentConverterConfig &config) {
  pImpl->config = config;
  return {.error = ErrorCode::SUCCESS};
}

Result<std::shared_ptr<Garment>>
GarmentConverter::convert(const ImageData &image, GarmentType type) {
  auto garment = std::make_shared<Garment>();
  cv::Mat cvImage(image.height, image.width, CV_8UC4, const_cast<uint8_t *>(image.pixels.data()));

  if (type == GarmentType::UNKNOWN) {
    type = pImpl->detectType(cvImage);
  }
  garment->setType(type);

  cv::Mat mask = pImpl->segmentGarment(cvImage);

  std::shared_ptr<Mesh> templateMesh = pImpl->tshirtTemplate;
  if (templateMesh) {
    auto deformedMesh = std::make_shared<Mesh>(*templateMesh);
    pImpl->fitMeshToSilhouette(deformedMesh, mask);
    garment->setMesh(deformedMesh);
    
    // リギング情報の生成 (TODO: Garmentクラスに保存用メンバを追加)
    auto weights = pImpl->rigToBody(garment->getMesh(), type);
  }

  auto texture = std::make_shared<Texture>();
  texture->loadFromMemory(image.pixels.data(), image.width, image.height, image.channels);
  garment->setTexture(texture);

  return {.value = garment, .error = ErrorCode::SUCCESS};
}

Result<std::shared_ptr<Garment>>
GarmentConverter::convertFromServer(const std::string &url) {
  return {.error = ErrorCode::NETWORK_ERROR, .message = "Not implemented"};
}

Result<void>
GarmentConverter::setupClothSimulation(std::shared_ptr<Garment> garment) {
  return {.error = ErrorCode::SUCCESS};
}

} // namespace arfit
