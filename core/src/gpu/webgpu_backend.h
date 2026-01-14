#pragma once

#include "gpu_backend.h"

#ifdef ARFIT_USE_WEBGPU
#include <vector>
#include <webgpu/webgpu.h>

namespace arfit {

class WebGPUContext : public IGPUContext {
public:
  WebGPUContext();
  ~WebGPUContext() override;

  bool initialize() override;
  std::shared_ptr<IGPUBuffer> createBuffer(size_t size,
                                           BufferType type) override;
  std::shared_ptr<IGPUShader> createShader(const std::string &source,
                                           ShaderType type) override;

  void beginFrame() override;
  void endFrame() override;

  void
  dispatch(std::shared_ptr<IGPUShader> shader, uint32_t x, uint32_t y,
           uint32_t z,
           const std::vector<std::shared_ptr<IGPUBuffer>> &bindings) override;

private:
  WGPUInstance instance = nullptr;
  WGPUAdapter adapter = nullptr;
  WGPUDevice device = nullptr;
  WGPUQueue queue = nullptr;

  WGPUCommandEncoder currentEncoder = nullptr;

  void requestAdapter();
  void requestDevice();
};

} // namespace arfit

#endif // ARFIT_USE_WEBGPU
