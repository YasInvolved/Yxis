#pragma once

#include "../internal_pch.h"
#include "Swapchain.h"
#include "TimelineSemaphore.h"
#include "vk_mem_alloc.h"

namespace Yxis::Vulkan
{
   struct Queue
   {
      uint32_t             familyIndex;
      std::vector<VkQueue> queues;
   };

   struct Queues
   {
      Queue                graphics;
      std::optional<Queue> compute;
      std::optional<Queue> transfer;
      std::optional<Queue> sparseBinding;
      std::optional<Queue> videoDecode;
      std::optional<Queue> videoEncode;
      std::optional<Queue> nvOpticalFlow;
   };

   class Device
   {
   public:
      Device(VkPhysicalDevice physicalDevice);
      ~Device();

      operator VkDevice() const;
      operator VkPhysicalDevice() const;

      Device(const Device&) = delete;
      Device& operator=(const Device&) = delete;

      const VkDevice getLogicalDevice() const;
      const VkPhysicalDevice getPhysicalDevice() const;
      const VkSurfaceCapabilities2KHR getSurfaceCapabilities() const;
      const std::vector<VkSurfaceFormat2KHR> getSurfaceFormats() const;
      const std::vector<VkPresentModeKHR> getPresentModes() const;

      // queues
      const Queues& getDeviceQueues() const;

      // synchronization
      const TimelineSemaphore createTimelineSemaphore() const;

      // memory
      const VmaAllocator getAllocator() const;

   private:
      VkDevice m_device;
      VkPhysicalDevice m_physicalDevice;

      struct {
         VmaAllocator allocator;
      } m_memoryManager;

      std::unique_ptr<Swapchain> m_swapchain;
      Queues m_queues;
   };
}