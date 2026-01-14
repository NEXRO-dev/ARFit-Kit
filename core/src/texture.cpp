/**
 * @file texture.cpp
 * @brief Texture implementation
 */

#include "texture.h"
#include <opencv2/opencv.hpp>

namespace arfit {

class Texture::Impl {
public:
  ImageData data;
  TextureFormat format = TextureFormat::RGBA8;
  TextureFilter filter = TextureFilter::LINEAR;
  TextureWrap wrap = TextureWrap::CLAMP;
  bool onGPU = false;
  bool hasMips = false;
  uint32_t textureId = 0;
};

Texture::Texture() : pImpl(std::make_unique<Impl>()) {}
Texture::~Texture() = default;

std::shared_ptr<Texture> Texture::fromImage(const ImageData &image) {
  auto texture = std::make_shared<Texture>();
  texture->pImpl->data = image;

  if (image.channels == 4) {
    texture->pImpl->format = TextureFormat::RGBA8;
  } else if (image.channels == 3) {
    texture->pImpl->format = TextureFormat::RGB8;
  } else if (image.channels == 1) {
    texture->pImpl->format = TextureFormat::R8;
  }

  return texture;
}

std::shared_ptr<Texture> Texture::fromFile(const std::string &path) {
  cv::Mat image = cv::imread(path, cv::IMREAD_UNCHANGED);

  if (image.empty()) {
    return nullptr;
  }

  // Convert to RGBA
  cv::Mat rgba;
  if (image.channels() == 4) {
    rgba = image;
  } else if (image.channels() == 3) {
    cv::cvtColor(image, rgba, cv::COLOR_BGR2BGRA);
  } else if (image.channels() == 1) {
    cv::cvtColor(image, rgba, cv::COLOR_GRAY2BGRA);
  } else {
    return nullptr;
  }

  // Convert BGR to RGB
  cv::cvtColor(rgba, rgba, cv::COLOR_BGRA2RGBA);

  ImageData imageData;
  imageData.width = rgba.cols;
  imageData.height = rgba.rows;
  imageData.channels = 4;
  imageData.pixels.resize(rgba.total() * rgba.elemSize());
  std::memcpy(imageData.pixels.data(), rgba.data, imageData.pixels.size());

  return fromImage(imageData);
}

std::shared_ptr<Texture> Texture::create(int width, int height,
                                         TextureFormat format) {
  auto texture = std::make_shared<Texture>();
  texture->pImpl->data.width = width;
  texture->pImpl->data.height = height;
  texture->pImpl->format = format;

  int channels = 4;
  switch (format) {
  case TextureFormat::RGBA8:
  case TextureFormat::RGBA16F:
    channels = 4;
    break;
  case TextureFormat::RGB8:
    channels = 3;
    break;
  case TextureFormat::R8:
  case TextureFormat::DEPTH24:
    channels = 1;
    break;
  }

  texture->pImpl->data.channels = channels;
  texture->pImpl->data.pixels.resize(width * height * channels, 0);

  return texture;
}

int Texture::getWidth() const { return pImpl->data.width; }

int Texture::getHeight() const { return pImpl->data.height; }

TextureFormat Texture::getFormat() const { return pImpl->format; }

void Texture::setFilter(TextureFilter filter) { pImpl->filter = filter; }

TextureFilter Texture::getFilter() const { return pImpl->filter; }

void Texture::setWrap(TextureWrap wrap) { pImpl->wrap = wrap; }

TextureWrap Texture::getWrap() const { return pImpl->wrap; }

void Texture::setData(const ImageData &image) { pImpl->data = image; }

ImageData Texture::getData() const { return pImpl->data; }

void Texture::uploadToGPU() {
  // TODO: Create GPU texture and upload data
  pImpl->onGPU = true;
}

void Texture::releaseGPU() {
  // TODO: Delete GPU texture
  pImpl->onGPU = false;
  pImpl->textureId = 0;
}

bool Texture::isOnGPU() const { return pImpl->onGPU; }

uint32_t Texture::getTextureId() const { return pImpl->textureId; }

void Texture::generateMipmaps() {
  // TODO: Generate mipmaps
  pImpl->hasMips = true;
}

bool Texture::hasMipmaps() const { return pImpl->hasMips; }

} // namespace arfit
