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

      // queues
      const QueueFamilyIndices& getQueueFamilyIndices() const;
      const VkQueue getQueue(QueueType type) const;
      const VkCommandPool getCommandPoolForQueueType(QueueType type) const;

      template <uint32_t count>
      const std::array<VkCommandBuffer, count> allocateCommandBuffers(QueueType queueType, const VkCommandBufferLevel level) const
      {
         VkCommandPool commandPool = getCommandPoolForQueueType(queueType);
         assert(commandPool != VK_NULL_HANDLE && "commandPool is null");
         std::array<VkCommandBuffer, count> commandBuffers;
         const VkCommandBufferAllocateInfo allocateInfo =
         {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = commandPool,
            .level = level,
            .commandBufferCount = count
         };

         VkResult result = vkAllocateCommandBuffers(m_device, &allocateInfo, commandBuffers.data());
         if (result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("Failed to create {} command buffers. {}", count, string_VkResult(result)));

         return commandBuffers;
      }

      void freeCommandBuffers(Device::QueueType type, const std::span<VkCommandBuffer> commandBuffers) const;

      // synchronization
      const VkFence createFence(bool signaled) const;
      const VkSemaphore craeteSemaphore() const;
      const VkSemaphore createTimelineSemaphore(uint64_t initialValue) const;
      void signalTimelineSemaphore(VkSemaphore semaphore, uint64_t newValue) const;
      inline bool checkTimelineSemaphoreCompletion(VkSemaphore semaphore, uint64_t expectedValue) const;
   private:
      VkDevice m_device;
      VkPhysicalDevice m_physicalDevice;

      struct {
         VkCommandPool gfxCommandPool;
         VkCommandPool computeCommandPool;
         VkCommandPool transferCommandPool;
      } m_commandPools;

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