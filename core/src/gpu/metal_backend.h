#pragma once

#include "gpu_backend.h"

#ifdef ARFIT_USE_METAL
#include <Metal/Metal.h>

namespace arfit {

class MetalContext : public IGPUContext {
public:
  MetalContext();
  ~MetalContext() override;

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
  id<MTLDevice> device;
  id<MTLCommandQueue> commandQueue;
  id<MTLCommandBuffer> currentCommandBuffer;
};

} // namespace arfit

#endif // ARFIT_USE_METAL
