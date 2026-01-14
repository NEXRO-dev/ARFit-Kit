#include "gpu_backend.h"

// Include backend headers conditionally
#ifdef ARFIT_USE_METAL
#include "metal_backend.h"
#endif

#ifdef ARFIT_USE_VULKAN
#include "vulkan_backend.h"
#endif

#ifdef ARFIT_USE_WEBGPU
#include "webgpu_backend.h"
#endif

#include <iostream>

namespace arfit {

/**
 * @brief Factory function to create the appropriate GPU context based on the
 * build target.
 *
 * Logic:
 * - If Metal is available (iOS/macOS), prefer Metal.
 * - If Vulkan is available (Android/Desktop), use Vulkan.
 * - If WebGPU is available (Web), use WebGPU.
 * - Fallback to nullptr (CPU only).
 */
std::unique_ptr<IGPUContext> createGPUContext() {
#ifdef ARFIT_USE_METAL
  std::cout << "[GPU] Initializing Metal Backend..." << std::endl;
  return std::make_unique<MetalContext>();
#elif defined(ARFIT_USE_VULKAN)
  std::cout << "[GPU] Initializing Vulkan Backend..." << std::endl;
  return std::make_unique<VulkanContext>();
#elif defined(ARFIT_USE_WEBGPU)
  std::cout << "[GPU] Initializing WebGPU Backend..." << std::endl;
  return std::make_unique<WebGPUContext>();
#else
  std::cerr << "[GPU] No supported GPU backend enabled. Falling back to CPU."
            << std::endl;
  return nullptr;
#endif
}

} // namespace arfit
