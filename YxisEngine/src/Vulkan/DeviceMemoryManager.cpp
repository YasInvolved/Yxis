#include "DeviceMemoryManager.h"
#include "Device.h"
#include "VulkanRenderer.h"
#include <Yxis/Application.h>
#include <Yxis/Logger.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace Yxis::Vulkan;

static constexpr VkDeviceSize STAGING_BUFFER_SIZE = 100 * 1024 * 1024; // 100mb
constexpr VmaAllocationCreateInfo STAGING_BUFFER_ALLOC_CREATE_INFO =
{
   .usage = VMA_MEMORY_USAGE_AUTO,
   .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
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

   {
      const VkCommandPoolCreateInfo createInfo =
      {
         .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
         .pNext = nullptr,
         .flags = 0,
         .queueFamilyIndex = m_device->getQueueFamilyIndices().transferIndex.value()
      };

      result = vkCreateCommandPool(logicalDevice, &createInfo, nullptr, &m_transferQueueCommandPool);
      if (result != VK_SUCCESS)
         throw std::runtime_error(fmt::format("Failed to create transfer command pool. {}", string_VkResult(result)));

      const VkCommandBufferAllocateInfo allocateInfo =
      {
         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
         .pNext = nullptr,
         .commandPool = m_transferQueueCommandPool,
         .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
         .commandBufferCount = 1,
      };

      result = vkAllocateCommandBuffers(logicalDevice, &allocateInfo, &m_transferCommandBuffer);
      if (result != VK_SUCCESS)
         throw std::runtime_error(fmt::format("Failed to allocate transfer command buffer. {}", string_VkResult(result)));

      const VkCommandBufferInheritanceInfo inheritanceInfo =
      {
         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
         .renderPass = VK_NULL_HANDLE,
         .subpass = 0,
         .framebuffer = VK_NULL_HANDLE,
         .occlusionQueryEnable = VK_FALSE,
         .queryFlags = 0,
         .pipelineStatistics = 0
      };

      const VkCommandBufferBeginInfo beginInfo =
      {
         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
         .pNext = nullptr,
         .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
         .pInheritanceInfo = &inheritanceInfo
      };

      vkBeginCommandBuffer(m_transferCommandBuffer, &beginInfo);

      const VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
      vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &m_transferFence);
   }

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

void DeviceMemoryManager::copyAssetToBuffer(const void* asset, VkBuffer buffer, std::optional<VkDeviceSize> size)
{
   VkDevice logicalDevice = m_device->getLogicalDevice();
   VkDeviceMemory stagingBufferMemory = m_stagingBufferMemory->GetMemory();
   ResourceKey key = std::make_pair(reinterpret_cast<uintptr_t>(buffer), ResourceType::BUFFER);
   auto& allocation = m_allocations[key];
   
   if (not size.has_value())
      size = allocation->GetSize();

   void* mappedBuffer;
   VkDeviceSize offset = 0;
   while (offset < size.value())
   {
      VkDeviceSize sizeToCopy = STAGING_BUFFER_SIZE < size.value() ? STAGING_BUFFER_SIZE : size.value();
      vkMapMemory(logicalDevice, stagingBufferMemory, 0, STAGING_BUFFER_SIZE, 0, &mappedBuffer);
      std::memcpy(mappedBuffer, asset, sizeToCopy);
      executeTransferBuffer(buffer, VkBufferCopy{ 0, offset, sizeToCopy });
      offset += sizeToCopy;
      size = size.value() - sizeToCopy;
   }

   vkUnmapMemory(logicalDevice, stagingBufferMemory);
}

void DeviceMemoryManager::executeTransferBuffer(VkBuffer buffer, const VkBufferCopy memRegion)
{
   VkDevice logicalDevice = m_device->getLogicalDevice();
   vkResetCommandBuffer(m_transferCommandBuffer, 0);
   vkCmdCopyBuffer(m_transferCommandBuffer, m_stagingBuffer, buffer, 1, &memRegion);
   vkEndCommandBuffer(m_transferCommandBuffer);

   const VkCommandBufferSubmitInfo commandBufferSubmitInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, nullptr, m_transferCommandBuffer, 0 };
   VkSubmitInfo2 submitInfo =
   {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .pNext = nullptr,
      .flags = 0,
      .waitSemaphoreInfoCount = 0,
      .pWaitSemaphoreInfos = nullptr,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &commandBufferSubmitInfo,
      .signalSemaphoreInfoCount = 0,
      .pSignalSemaphoreInfos = nullptr
   };
   vkQueueSubmit2(m_device->getQueue(Device::QueueType::TRANSFER), 1, &submitInfo, m_transferFence);
   vkWaitForFences(logicalDevice, 1, &m_transferFence, VK_TRUE, 0);
   vkResetFences(logicalDevice, 1, &m_transferFence);
}

void DeviceMemoryManager::copyAssetToBuffer(const void* asset, VkImage image, std::optional<VkDeviceSize> size)
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

DeviceMemoryManager::~DeviceMemoryManager()
{
   VkDevice logicalDevice = m_device->getLogicalDevice();
   vkDestroyFence(logicalDevice, m_transferFence, nullptr);
   vkFreeCommandBuffers(logicalDevice, m_transferQueueCommandPool, 1, &m_transferCommandBuffer);
   vkDestroyCommandPool(logicalDevice , m_transferQueueCommandPool, nullptr);
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