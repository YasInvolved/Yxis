#include "DeviceMemoryManager.h"
#include "Device.h"
#include "VulkanRenderer.h"
#include <Yxis/Application.h>
#include <Yxis/Logger.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace Yxis::Vulkan;

static constexpr uint32_t STAGING_BUFFER_SIZE = 100 * 1024 * 1024;
constexpr VmaAllocationCreateInfo STAGING_BUFFER_ALLOC_CREATE_INFO =
{
   .usage = VMA_MEMORY_USAGE_AUTO,
   .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
};

DeviceMemoryManager::DeviceMemoryManager(const Device* device)
   : m_device(device)
{
   const VkPhysicalDevice physicalDevice = m_device->getPhysicalDevice();
   vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &m_memoryProperties);

   VmaVulkanFunctions vulkanFunctions =
   {
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr
   };

   VmaAllocatorCreateInfo allocatorCreateInfo =
   {
      .flags = VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT,
      .physicalDevice = physicalDevice,
      .device = m_device->getLogicalDevice(),
      .pVulkanFunctions = &vulkanFunctions,
      .instance = VulkanRenderer::getInstance(),
      .vulkanApiVersion = VK_API_VERSION_1_4,
   };

   VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create device memory allocator. {}", string_VkResult(result)));

   {
      const VkBufferCreateInfo createInfo =
      {
         .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
         .pNext = nullptr,
         .flags = 0,
         .size = STAGING_BUFFER_SIZE,
         .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
         .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
         .queueFamilyIndexCount = 1,
         .pQueueFamilyIndices = &device->getQueueFamilyIndices().transferIndex.value(),
      };

      result = vmaCreateBuffer(m_allocator, &createInfo, &STAGING_BUFFER_ALLOC_CREATE_INFO, &m_stagingBuffer, &m_stagingBufferMemory, nullptr);
      if (result != VK_SUCCESS)
         throw std::runtime_error(fmt::format("Failed to create staging buffer. {}", string_VkResult(result)));

#ifdef YX_DEBUG
      const VkDebugMarkerObjectNameInfoEXT debugMarker =
      {
         .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
         .pNext = nullptr,
         .objectType = VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
         .object = reinterpret_cast<uint64_t>(m_stagingBuffer),
         .pObjectName = "MEM_MGR_STAGING_BUFFER"
      };

      result = vkDebugMarkerSetObjectNameEXT(device->getLogicalDevice(), &debugMarker);
      if (result != VK_SUCCESS)
         YX_CORE_LOGGER->warn("Failed to set debug name for Memory Manager Staging Buffer. {}", string_VkResult(result));
#endif
   }
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

DeviceMemoryManager::~DeviceMemoryManager()
{
   vmaDestroyBuffer(m_allocator, m_stagingBuffer, m_stagingBufferMemory);

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