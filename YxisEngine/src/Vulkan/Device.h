#pragma once

#include "../internal_pch.h"
#include "Swapchain.h"
#include "DeviceMemoryManager.h"

namespace Yxis::Vulkan
{
   struct QueueFamilyIndices
   {
      uint32_t                gfxIndex;
      std::optional<uint32_t> computeIndex;
      std::optional<uint32_t> transferIndex;
      std::optional<uint32_t> sparseBindingIndex;
      std::optional<uint32_t> opticalFlowIndex;
   };

   class Device
   {
   public:
      enum class QueueType { GRAPHICS, COMPUTE, TRANSFER, SPARSE_BINDING, OPTICAL_FLOW, VIDEO_DECODE, VIDEO_ENCODE };
      Device(VkPhysicalDevice physicalDevice, QueueFamilyIndices&& queueIndices);
      ~Device();

      Device(const Device&) = delete;
      Device& operator=(const Device&) = delete;

      const VkDevice getLogicalDevice() const;
      const VkPhysicalDevice getPhysicalDevice() const;
      const VkSurfaceCapabilities2KHR getSurfaceCapabilities() const;
      const std::vector<VkSurfaceFormat2KHR> getSurfaceFormats() const;
      const std::vector<VkPresentModeKHR> getPresentModes() const;
      const QueueFamilyIndices& getQueueFamilyIndices() const;
      const VkQueue getQueue(QueueType type) const;
   private:
      VkDevice m_device;
      VkPhysicalDevice m_physicalDevice;
      VkCommandPool m_commandPool;

      std::unique_ptr<Swapchain> m_swapchain;
      std::unique_ptr<DeviceMemoryManager> m_memoryManager;

      struct {
         VkQueue graphicsQueue = VK_NULL_HANDLE;
         VkQueue computeQueue = VK_NULL_HANDLE;
         VkQueue transferQueue = VK_NULL_HANDLE;
      } m_queues;

      QueueFamilyIndices m_queueFamilies;
   };
}