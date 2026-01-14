#include "vulkan_backend.h"
#include <cstring>
#include <iostream>

#ifdef ARFIT_USE_VULKAN

namespace arfit {

class VulkanBuffer : public IGPUBuffer {
public:
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkDevice device;
  size_t size;

  VulkanBuffer(VkDevice dev, size_t sz) : device(dev), size(sz) {
    // NOTE: Simplified buffer creation. Real implementation needs memory type
    // selection.
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
      std::cerr << "Failed to create Vulkan buffer" << std::endl;
    }
    // Memory allocation omitted for brevity (requires finding memory type
    // index)
  }

  ~VulkanBuffer() {
    if (buffer)
      vkDestroyBuffer(device, buffer, nullptr);
    if (memory)
      vkFreeMemory(device, memory, nullptr);
  }

  void upload(const void *data, size_t size) override {
    // Needs mapping memory
  }

  void download(void *data, size_t size) override {
    // Needs mapping memory
  }

  size_t getSize() const override { return size; }
};

class VulkanShader : public IGPUShader {
public:
  VkShaderModule shaderModule;
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

VulkanContext::VulkanContext() {}

VulkanContext::~VulkanContext() {
  if (device)
    vkDestroyDevice(device, nullptr);
  if (instance)
    vkDestroyInstance(instance, nullptr);
}

bool VulkanContext::initialize() {
  if (!createInstance())
    return false;
  if (!selectPhysicalDevice())
    return false;
  if (!createLogicalDevice())
    return false;
  return createCommandPool();
}

bool VulkanContext::createInstance() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "ARFitKit";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "ARFitKit Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_1;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  return vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS;
}

bool VulkanContext::selectPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0)
    return false;

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  physicalDevice = devices[0];
  return true;
}

bool VulkanContext::createLogicalDevice() {
  computeQueueFamilyIndex = 0;

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    return false;
  }

  vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);
  return true;
}

bool VulkanContext::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = computeQueueFamilyIndex;
  return vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) ==
         VK_SUCCESS;
}

std::shared_ptr<IGPUBuffer> VulkanContext::createBuffer(size_t size,
                                                        BufferType type) {
  return std::make_shared<VulkanBuffer>(device, size);
}

std::shared_ptr<IGPUShader>
VulkanContext::createShader(const std::string &source, ShaderType type) {
  return std::make_shared<VulkanShader>();
}

void VulkanContext::beginFrame() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

void VulkanContext::endFrame() {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(computeQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanContext::dispatch(
    std::shared_ptr<IGPUShader> shader, uint32_t x, uint32_t y, uint32_t z,
    const std::vector<std::shared_ptr<IGPUBuffer>> &bindings) {
  auto vkShader = std::static_pointer_cast<VulkanShader>(shader);

  vkCmdDispatch(commandBuffer, x, y, z);
}

} // namespace arfit

#endif // ARFIT_USE_VULKAN
