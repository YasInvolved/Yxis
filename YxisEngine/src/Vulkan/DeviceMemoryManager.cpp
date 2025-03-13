#include "DeviceMemoryManager.h"
#include "Device.h"
#include "VulkanRenderer.h"
#include <Yxis/Application.h>
#include <Yxis/Logger.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "VulkanUtilities.h"

using namespace Yxis::Vulkan;

// it doesn't change and multiple functions use it, so it's better to leave it outside
static constexpr VkCommandBufferBeginInfo s_transferCmdBufferBeginInfo =
{
   .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
   .pNext = nullptr,
   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
   .pInheritanceInfo = nullptr
};

DeviceMemoryManager::DeviceMemoryManager(const Device* device)
   : m_device(device)
{
   const VkDevice logicalDevice = m_device->getLogicalDevice();
   const VkPhysicalDevice physicalDevice = m_device->getPhysicalDevice();
   vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &m_memoryProperties);

   VmaVulkanFunctions vulkanFunctions =
   {
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr
   };

   VmaAllocatorCreateInfo allocatorCreateInfo =
   {
      .flags = 0,
      .physicalDevice = physicalDevice,
      .device = m_device->getLogicalDevice(),
      .pVulkanFunctions = &vulkanFunctions,
      .instance = VulkanRenderer::getInstance(),
      .vulkanApiVersion = VK_API_VERSION_1_4,
   };

   VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create device memory allocator. {}", string_VkResult(result)));

   m_stagingBuffer = std::make_unique<StagingBuffer>(this);
}

VkBuffer DeviceMemoryManager::createBuffer(VkBufferCreateInfo& createInfo)
{
   if ((createInfo.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != VK_BUFFER_USAGE_TRANSFER_DST_BIT)
      createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

   const VmaAllocationCreateInfo allocInfo =
   {
      .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
   };

   VmaAllocation allocation;
   VkBuffer buffer;
   VkResult result = vmaCreateBuffer(m_allocator, &createInfo, &allocInfo, &buffer, &allocation, nullptr);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create a buffer. {}", string_VkResult(result)));

   ResourceKey key = std::make_pair(reinterpret_cast<uintptr_t>(buffer), ResourceType::BUFFER);
   m_allocations.emplace(std::move(key), std::move(allocation));

   return buffer;
}

VkImage DeviceMemoryManager::createImage(VkImageCreateInfo& createInfo)
{
   const VmaAllocationCreateInfo allocInfo =
   {
      .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
   };

   VmaAllocation allocation;
   VkImage image;
   VkResult result = vmaCreateImage(m_allocator, &createInfo, &allocInfo, &image, &allocation, nullptr);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create a buffer. {}", string_VkResult(result)));

   ResourceKey key = std::make_pair(reinterpret_cast<uintptr_t>(image), ResourceType::IMAGE);
   m_allocations.emplace(std::move(key), std::move(allocation));

   return image;
}

void DeviceMemoryManager::deleteAsset(VkBuffer buffer)
{
   ResourceKey key = std::make_pair(reinterpret_cast<uintptr_t>(buffer), ResourceType::BUFFER);
   auto& allocation = m_allocations[key];
   vmaDestroyBuffer(m_allocator, buffer, allocation);
}

void DeviceMemoryManager::deleteAsset(VkImage image)
{
   ResourceKey key = std::make_pair(reinterpret_cast<uintptr_t>(image), ResourceType::IMAGE);
   auto& allocation = m_allocations[key];
   vmaDestroyImage(m_allocator, image, allocation);
}

void DeviceMemoryManager::copyToBuffer(const void* asset, VkBuffer buffer, std::optional<VkDeviceSize> size)
{
   if (not size.has_value())
   {
      VkMemoryRequirements memRequirements;
      vkGetBufferMemoryRequirements(*m_device, buffer, &memRequirements);
      size = memRequirements.size;
   }

   m_stagingBuffer->copyToBuffer(asset, buffer, size.value());
}

void DeviceMemoryManager::copyToImage(const void* asset, VkImage image, VkExtent3D extent)
{
}

// STAGING BUFFER
DeviceMemoryManager::StagingBuffer::StagingBuffer(DeviceMemoryManager* memoryManager)
   : m_memoryManager(memoryManager)
{
   const VkBufferCreateInfo createInfo =
   {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = s_sbSize,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &m_memoryManager->m_device->getQueueFamilyIndices().transferIndex.value(),
   };

   constexpr VmaAllocationCreateInfo allocCreateInfo =
   {
      .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
      .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
   };

   VmaAllocationInfo allocInfo;
   VkResult result = vmaCreateBuffer(m_memoryManager->m_allocator, &createInfo, &allocCreateInfo, &m_stagingBuffer, &m_sbAllocation, &allocInfo);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create staging buffer. {}", string_VkResult(result)));

   m_memPtr = allocInfo.pMappedData;
   m_transferCommandBuffers = m_memoryManager->m_device->allocateCommandBuffers<1>(Device::QueueType::TRANSFER, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
   m_transferQueue = m_memoryManager->m_device->getQueue(Device::QueueType::TRANSFER);
}

void DeviceMemoryManager::StagingBuffer::copyToBuffer(const void* data, VkBuffer buffer, VkDeviceSize size)
{
   auto* device = m_memoryManager->m_device;

   VkSemaphore sbSemaphore = device->createTimelineSemaphore(0);
   uint64_t timelineValue = 0;

   do
   {
      device->waitForSemaphore(sbSemaphore, timelineValue);

      // reset command buffer
      vkResetCommandBuffer(m_transferCommandBuffers[0], 0);

      size_t opSize = size < s_sbChunkSize ? size : s_sbChunkSize;
      size_t sbOffset = (timelineValue % s_sbChunks) * s_sbChunkSize;
      size_t bufferOffset = timelineValue * s_sbChunkSize;
      std::memcpy(static_cast<uint8_t*>(m_memPtr) + sbOffset, static_cast<const uint8_t*>(data) + bufferOffset, opSize); // copy to the staging buffer

      // gpu work
      vkBeginCommandBuffer(m_transferCommandBuffers[0], &s_transferCmdBufferBeginInfo);
      const VkBufferCopy copyRegion{ sbOffset, bufferOffset, opSize };
      vkCmdCopyBuffer(m_transferCommandBuffers[0], m_stagingBuffer, buffer, 1, &copyRegion);
      vkEndCommandBuffer(m_transferCommandBuffers[0]);

      timelineValue++;
      const VkTimelineSemaphoreSubmitInfo semaphoreSubmitInfo =
      {
         .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
         .pNext = nullptr,
         .waitSemaphoreValueCount = 0,
         .pWaitSemaphoreValues = nullptr,
         .signalSemaphoreValueCount = 1,
         .pSignalSemaphoreValues = &timelineValue
      };

      const VkSubmitInfo submitInfo =
      {
         .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
         .pNext = &semaphoreSubmitInfo,
         .waitSemaphoreCount = 0,
         .pWaitSemaphores = nullptr,
         .commandBufferCount = 1,
         .pCommandBuffers = &m_transferCommandBuffers[0],
         .signalSemaphoreCount = 1,
         .pSignalSemaphores = &sbSemaphore,
      };

      vkQueueSubmit(m_transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
      size -= opSize;
   } while (size > 0);

   // ensure everything is completed
   device->waitForSemaphore(sbSemaphore, timelineValue);
   vkDestroySemaphore(*device, sbSemaphore, nullptr);
}

void DeviceMemoryManager::StagingBuffer::copyToImage(const void* data, VkImage image, VkExtent3D extent, VkDeviceSize size)
{
   YX_CORE_LOGGER->error("Not implemented");
}

DeviceMemoryManager::StagingBuffer::~StagingBuffer()
{
   vmaDestroyBuffer(m_memoryManager->m_allocator, m_stagingBuffer, m_sbAllocation);
   m_memoryManager->m_device->freeCommandBuffers(Device::QueueType::TRANSFER, m_transferCommandBuffers);
}

DeviceMemoryManager::~DeviceMemoryManager()
{
   m_stagingBuffer.reset();
   VkDevice logicalDevice = m_device->getLogicalDevice();
   if (m_allocations.size() > 0)
   {
      for (const auto& [resourceKey, allocation] : m_allocations)
         if (resourceKey.second == ResourceType::BUFFER)
            vmaDestroyBuffer(m_allocator, reinterpret_cast<VkBuffer>(resourceKey.first), allocation);
         else
            vmaDestroyImage(m_allocator, reinterpret_cast<VkImage>(resourceKey.first), allocation);
   }

   if (m_allocator != nullptr)
      vmaDestroyAllocator(m_allocator);
}