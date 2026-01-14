#pragma once

#include "gpu_backend.h"

#ifdef ARFIT_USE_VULKAN
#include <vector>
#include <vulkan/vulkan.h>

namespace arfit {

class VulkanContext : public IGPUContext {
public:
  VulkanContext();
  ~VulkanContext() override;

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

  // Helper to get raw devices if needed
  VkDevice getDevice() const { return device; }

private:
  bool createInstance();
  bool selectPhysicalDevice();
  bool createLogicalDevice();
  bool createCommandPool();

  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue computeQueue = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  uint32_t computeQueueFamilyIndex = -1;
};

} // namespace arfit

#endif // ARFIT_USE_VULKAN
