/**
 * @file texture.h
 * @brief Texture handling for garment rendering
 */

#pragma once

#include "types.h"
#include <memory>
#include <string>

namespace arfit {

/**
 * @brief Texture format
 */
enum class TextureFormat { RGBA8, RGB8, R8, RGBA16F, DEPTH24 };

/**
 * @brief Texture filter mode
 */
enum class TextureFilter { NEAREST, LINEAR, TRILINEAR };

/**
 * @brief Texture wrap mode
 */
enum class TextureWrap { REPEAT, CLAMP, MIRROR };

/**
 * @brief Texture class for garment textures
 */
class Texture {
public:
  Texture();
  ~Texture();

  /**
   * @brief Create texture from image data
   * @param image Source image data
   * @return Created texture
   */
  static std::shared_ptr<Texture> fromImage(const ImageData &image);

  /**
   * @brief Load texture from file
   * @param path File path
   * @return Loaded texture or nullptr on failure
   */
  static std::shared_ptr<Texture> fromFile(const std::string &path);

  /**
   * @brief Create empty texture with specified size
   * @param width Texture width
   * @param height Texture height
   * @param format Texture format
   * @return Created texture
   */
  static std::shared_ptr<Texture>
  create(int width, int height, TextureFormat format = TextureFormat::RGBA8);

  // Properties
  int getWidth() const;
  int getHeight() const;
  TextureFormat getFormat() const;

  // Filtering
  void setFilter(TextureFilter filter);
  TextureFilter getFilter() const;

  // Wrapping
  void setWrap(TextureWrap wrap);
  TextureWrap getWrap() const;

  // Data access
  void setData(const ImageData &image);
  ImageData getData() const;

  // GPU management
  void uploadToGPU();
  void releaseGPU();
  bool isOnGPU() const;
  uint32_t getTextureId() const;

  // Mipmaps
  void generateMipmaps();
  bool hasMipmaps() const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace arfit
