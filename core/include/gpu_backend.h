/**
 * @file gpu_backend.h
 * @brief Abstract interface for GPU backends (Metal, Vulkan)
 */

#pragma once

#include "types.h"
#include <memory>
#include <string>
#include <vector>

namespace arfit {

enum class BufferType { VERTEX, INDEX, UNIFORM, STORAGE };

enum class ShaderType { VERTEX_FRAGMENT, COMPUTE };

// Abstract GPU Buffer
class IGPUBuffer {
public:
  virtual ~IGPUBuffer() = default;
  virtual void upload(const void *data, size_t size) = 0;
  virtual void download(void *data, size_t size) = 0;
  virtual size_t getSize() const = 0;
};

// Abstract Shader
class IGPUShader {
public:
  virtual ~IGPUShader() = default;
};

// Abstract GPU Context
class IGPUContext {
public:
  virtual ~IGPUContext() = default;

  virtual bool initialize() = 0;

  // Buffer creation
  virtual std::shared_ptr<IGPUBuffer> createBuffer(size_t size,
                                                   BufferType type) = 0;

  // Shader creation
  virtual std::shared_ptr<IGPUShader> createShader(const std::string &source,
                                                   ShaderType type) = 0;

  // Command execution
  virtual void beginFrame() = 0;
  virtual void endFrame() = 0;

  // Compute dispatch
  virtual void
  dispatch(std::shared_ptr<IGPUShader> shader, uint32_t x, uint32_t y,
           uint32_t z,
           const std::vector<std::shared_ptr<IGPUBuffer>> &bindings) = 0;
};

// Factory for creating backend-specific context
std::unique_ptr<IGPUContext> createGPUContext();

} // namespace arfit
