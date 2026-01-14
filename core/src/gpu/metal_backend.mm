#include "metal_backend.h"
#include <iostream>

#ifdef ARFIT_USE_METAL

namespace arfit {

class MetalBuffer : public IGPUBuffer {
public:
  id<MTLBuffer> buffer;
  size_t size;

  MetalBuffer(id<MTLDevice> device, size_t size) : size(size) {
    buffer = [device newBufferWithLength:size
                                 options:MTLResourceStorageModeShared];
  }

  void upload(const void *data, size_t size) override {
    if (size > this->size)
      return;
    memcpy(buffer.contents, data, size);
  }

  void download(void *data, size_t size) override {
    if (size > this->size)
      return;
    memcpy(data, buffer.contents, size);
  }

  size_t getSize() const override { return size; }
};

class MetalShader : public IGPUShader {
public:
  id<MTLComputePipelineState> pipelineState;
  // For compute shaders
};

MetalContext::MetalContext() { device = MTLCreateSystemDefaultDevice(); }

MetalContext::~MetalContext() {}

bool MetalContext::initialize() {
  if (!device)
    return false;
  commandQueue = [device newCommandQueue];
  return commandQueue != nil;
}

std::shared_ptr<IGPUBuffer> MetalContext::createBuffer(size_t size,
                                                       BufferType type) {
  return std::make_shared<MetalBuffer>(device, size);
}

std::shared_ptr<IGPUShader>
MetalContext::createShader(const std::string &source, ShaderType type) {
  // Compile shader from source (MSL) or load library
  // Simulating success for now
  return std::make_shared<MetalShader>();
}

void MetalContext::beginFrame() {
  currentCommandBuffer = [commandQueue commandBuffer];
}

void MetalContext::endFrame() {
  [currentCommandBuffer commit];
  // [currentCommandBuffer waitUntilCompleted]; // Optional sync
  currentCommandBuffer = nil;
}

void MetalContext::dispatch(
    std::shared_ptr<IGPUShader> shader, uint32_t x, uint32_t y, uint32_t z,
    const std::vector<std::shared_ptr<IGPUBuffer>> &bindings) {
  if (!currentCommandBuffer)
    return;

  id<MTLComputeCommandEncoder> encoder =
      [currentCommandBuffer computeCommandEncoder];

  auto mtlShader = std::static_pointer_cast<MetalShader>(shader);
  // [encoder setComputePipelineState:mtlShader->pipelineState];

  for (size_t i = 0; i < bindings.size(); ++i) {
    auto mtlBuffer = std::static_pointer_cast<MetalBuffer>(bindings[i]);
    [encoder setBuffer:mtlBuffer->buffer offset:0 atIndex:i];
  }

  MTLSize gridSize = MTLSizeMake(x, y, z);
  MTLSize threadGroupSize = MTLSizeMake(32, 1, 1); // Simplification

  // [encoder dispatchThreadgroups:gridSize
  // threadsPerThreadgroup:threadGroupSize];
  [encoder endEncoding];
}

std::unique_ptr<IGPUContext> createGPUContext() {
  return std::make_unique<MetalContext>();
}

} // namespace arfit

#else

namespace arfit {
std::unique_ptr<IGPUContext> createGPUContext() { return nullptr; }
} // namespace arfit

#endif // ARFIT_USE_METAL
