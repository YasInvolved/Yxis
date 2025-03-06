#include "DeviceMemoryManager.h"
#include "Device.h"
#include "VulkanRenderer.h"
#include <Yxis/Application.h>
#include <Yxis/Logger.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace Yxis::Vulkan;

static constexpr VkDeviceSize s_stagingBufferSize = 256 * 1024 * 1024; // 256mb

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

   m_stagingBuffer = std::make_unique<StagingBuffer>(this, s_stagingBufferSize);
}

VkBuffer DeviceMemoryManager::createBuffer(const VkBufferCreateInfo& createInfo)
{
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

VkImage DeviceMemoryManager::createImage(const VkImageCreateInfo& createInfo)
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

void DeviceMemoryManager::copyToBuffer(const void* asset, VkBuffer buffer, std::optional<VkDeviceSize> size)
{

}

void DeviceMemoryManager::copyToImage(const void* asset, VkImage image, std::optional<VkDeviceSize> size)
{

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

// STAGING BUFFER
DeviceMemoryManager::StagingBuffer::StagingBuffer(DeviceMemoryManager* memoryManager, VkDeviceSize size)
   : m_memoryManager(memoryManager), m_size(size)
{
   const VkBufferCreateInfo createInfo =
   {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = size,
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
}

void DeviceMemoryManager::StagingBuffer::copyToBuffer()
{

}

void DeviceMemoryManager::StagingBuffer::copyToImage()
{

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