#pragma once

#include "../internal_pch.h"

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
      Device(VkPhysicalDevice physicalDevice, QueueFamilyIndices&& queueIndices);
      ~Device();

      Device(const Device&) = delete;
      Device& operator=(const Device&) = delete;
   private:
      VkDevice m_device;
      VkPhysicalDevice m_physicalDevice;

      struct {
         VkQueue graphicsQueue = VK_NULL_HANDLE;
         VkQueue computeQueue = VK_NULL_HANDLE;
         VkQueue transferQueue = VK_NULL_HANDLE;
      } m_queues;

      QueueFamilyIndices m_queueFamilies;
   };
}