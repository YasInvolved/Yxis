#include "Device.h"
#include <Yxis/Logger.h>
#include "../Window.h"
#include "VulkanUtilities.h"

using namespace Yxis::Vulkan;

Device::Device(VkPhysicalDevice physicalDevice, QueueFamilyIndices&& queueIndices)
   : m_physicalDevice(physicalDevice), m_queueFamilies(std::move(queueIndices))
{
   constexpr float queuePriority = 1.0f; // TODO: make queue priority matter
   std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
   queueCreateInfos.emplace_back(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, m_queueFamilies.gfxIndex, 1, &queuePriority);

   if (m_queueFamilies.computeIndex.has_value())
      queueCreateInfos.emplace_back(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, m_queueFamilies.computeIndex.value(), 1, &queuePriority);
   
   if (m_queueFamilies.transferIndex.has_value())
      queueCreateInfos.emplace_back(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, m_queueFamilies.transferIndex.value(), 1, &queuePriority);

   constexpr std::array<const char*, 0> deviceEnabledLayers = {};
   constexpr const char* deviceEnabledExtensions[] = { 
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
#ifdef YX_DEBUG
      VK_EXT_DEBUG_MARKER_EXTENSION_NAME
#endif
   };

   
   VkPhysicalDeviceFeatures2 features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

   // link structures (vk14 -> vk13 -> vk12 -> vk11)
   VkPhysicalDeviceVulkan11Features vulkan11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
   VkPhysicalDeviceVulkan12Features vulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &vulkan11Features };
   VkPhysicalDeviceVulkan13Features vulkan13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, &vulkan12Features };
   VkPhysicalDeviceVulkan14Features vulkan14Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, &vulkan13Features };
   features.pNext = &vulkan14Features;

   vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

   VkDeviceCreateInfo deviceCreateInfo =
   {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &features,
      .flags = 0,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = static_cast<uint32_t>(deviceEnabledLayers.size()),
      .ppEnabledLayerNames = deviceEnabledLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(std::size(deviceEnabledExtensions)),
      .ppEnabledExtensionNames = deviceEnabledExtensions,
   };

   VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
   if (result != VK_SUCCESS)
   {
      throw std::runtime_error(fmt::format("Failed to create logical device. {}", string_VkResult(result)));
   }

   volkLoadDevice(m_device);

   VkDeviceQueueInfo2 queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2, nullptr, 0, m_queueFamilies.gfxIndex, 0 }; // graphics goes first

   vkGetDeviceQueue2(m_device, &queueInfo, &m_queues.graphicsQueue);
   if (m_queueFamilies.computeIndex.has_value())
   {
      assert(m_queues.computeQueue == VK_NULL_HANDLE && "The field of compute queue has been initialized already!");
      queueInfo.queueFamilyIndex = m_queueFamilies.computeIndex.value();
      vkGetDeviceQueue2(m_device, &queueInfo, &m_queues.computeQueue);
   }

   if (m_queueFamilies.transferIndex.has_value())
   {
      assert(m_queues.transferQueue == VK_NULL_HANDLE && "The field of transfer queue has been initialized already!");
      queueInfo.queueFamilyIndex = m_queueFamilies.transferIndex.value();
      vkGetDeviceQueue2(m_device, &queueInfo, &m_queues.transferQueue);
   }

   {
      VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, 0, m_queueFamilies.gfxIndex };
      result = vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPools.gfxCommandPool);
      if (result != VK_SUCCESS)
         throw std::runtime_error(fmt::format("Failed to create a graphics command pool. {}", string_VkResult(result)));

      commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.computeIndex.value();
      result = vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPools.computeCommandPool);
      if (result != VK_SUCCESS)
         throw std::runtime_error(fmt::format("Failed to create a compute command pool. {}", string_VkResult(result)));

      commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.transferIndex.value();
      result = vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPools.transferCommandPool);
      if (result != VK_SUCCESS)
         throw std::runtime_error(fmt::format("Failed to create a transfer command pool. {}", string_VkResult(result)));
   }

   m_swapchain = std::make_unique<Swapchain>(this);
   m_memoryManager = std::make_unique<DeviceMemoryManager>(this);
}

const VkDevice Device::getLogicalDevice() const
{
   return m_device;
}

const VkPhysicalDevice Device::getPhysicalDevice() const
{
   return m_physicalDevice;
}

const VkSurfaceCapabilities2KHR Device::getSurfaceCapabilities() const
{
   const VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo =
   {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
      .pNext = nullptr,
      .surface = Window::getSurface(),
   };
   VkSurfaceCapabilities2KHR surfaceCapabilites{ VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

   VkResult result = vkGetPhysicalDeviceSurfaceCapabilities2KHR(m_physicalDevice, &surfaceInfo, &surfaceCapabilites);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to get surface capabilites. {}", string_VkResult(result)));

   return surfaceCapabilites;
}

const std::vector<VkSurfaceFormat2KHR> Device::getSurfaceFormats() const
{
   uint32_t formatsCount;
   const VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo =
   {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
      .pNext = nullptr,
      .surface = Window::getSurface(),
   };

   vkGetPhysicalDeviceSurfaceFormats2KHR(m_physicalDevice, &surfaceInfo, &formatsCount, nullptr);
   std::vector<VkSurfaceFormat2KHR> formats(formatsCount, VkSurfaceFormat2KHR{ VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR });
   VkResult result = vkGetPhysicalDeviceSurfaceFormats2KHR(m_physicalDevice, &surfaceInfo, &formatsCount, formats.data());

   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to get available surface formats. {}", string_VkResult(result)));

   return formats;
}

const std::vector<VkPresentModeKHR> Device::getPresentModes() const
{
   uint32_t presentModesCount;
   const VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo =
   {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
      .pNext = nullptr,
      .surface = Window::getSurface(),
   };

   vkGetPhysicalDeviceSurfacePresentModes2EXT(m_physicalDevice, &surfaceInfo, &presentModesCount, nullptr);
   std::vector<VkPresentModeKHR> presentModes(presentModesCount);
   VkResult result = vkGetPhysicalDeviceSurfacePresentModes2EXT(m_physicalDevice, &surfaceInfo, &presentModesCount, presentModes.data());
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to get available present modes. {}", string_VkResult(result)));

   return presentModes;
}

const QueueFamilyIndices& Device::getQueueFamilyIndices() const
{
   return m_queueFamilies;
}

const VkQueue Device::getQueue(Device::QueueType type) const
{
   switch (type)
   {
   case QueueType::GRAPHICS:
      return m_queues.graphicsQueue;
      break;
   case QueueType::COMPUTE:
      return m_queues.computeQueue;
      break;
   case QueueType::TRANSFER:
      return m_queues.transferQueue;
      break;
   default:
      YX_CORE_LOGGER->warn("Requested queue type is not supported yet");
      break;
   }

   return VK_NULL_HANDLE;
}

const VkCommandPool Device::getCommandPoolForQueueType(Device::QueueType type) const
{
   switch (type)
   {
   case QueueType::GRAPHICS:
      return m_commandPools.gfxCommandPool;
      break;
   case QueueType::COMPUTE:
      return m_commandPools.computeCommandPool;
      break;
   case QueueType::TRANSFER:
      return m_commandPools.transferCommandPool;
      break;
   default:
      return VK_NULL_HANDLE;
      break;
   }
}

void Device::freeCommandBuffers(Device::QueueType type, const std::span<VkCommandBuffer> commandBuffers) const
{
   vkFreeCommandBuffers(m_device, getCommandPoolForQueueType(type), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
}

const VkFence Device::createFence(bool signaled) const
{
   VkFence fence;
   const VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, signaled ? VK_FENCE_CREATE_SIGNALED_BIT : static_cast<VkFenceCreateFlags>(0) };
   VkResult result = vkCreateFence(m_device, &createInfo, nullptr, &fence);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create a fence. {}", string_VkResult(result)));

   return fence;
}

const VkSemaphore Device::craeteSemaphore() const
{
   VkSemaphore semaphore;
   const VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
   VkResult result = vkCreateSemaphore(m_device, &createInfo, nullptr, &semaphore);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create a semaphore. {}", string_VkResult(result)));

   return semaphore;
}

const VkSemaphore Device::createTimelineSemaphore(uint64_t initialValue) const
{
   VkSemaphore semaphore;
   const VkSemaphoreTypeCreateInfo typeInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, nullptr, VK_SEMAPHORE_TYPE_TIMELINE, initialValue };
   const VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &typeInfo, 0 };
   VkResult result = vkCreateSemaphore(m_device, &createInfo, nullptr, &semaphore);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create a timeline semaphore. {}", string_VkResult(result)));

   return semaphore;
}

void Device::signalTimelineSemaphore(VkSemaphore semaphore, uint64_t newValue) const
{
   const VkSemaphoreSignalInfo signalInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO, nullptr, semaphore, newValue };
   VkResult result = vkSignalSemaphore(m_device, &signalInfo);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to signal a timeline semaphore. {}", string_VkResult(result)));
}

inline bool Device::checkTimelineSemaphoreCompletion(VkSemaphore semaphore, uint64_t expectedValue) const
{
   uint64_t currentValue;
   vkGetSemaphoreCounterValue(m_device, semaphore, &currentValue);
   return currentValue == expectedValue;
}

Device::~Device()
{
   m_memoryManager.reset();
   m_swapchain.reset();
   vkDestroyCommandPool(m_device, m_commandPools.gfxCommandPool, nullptr);
   vkDestroyCommandPool(m_device, m_commandPools.computeCommandPool, nullptr);
   vkDestroyCommandPool(m_device, m_commandPools.transferCommandPool, nullptr);
   if (m_device != VK_NULL_HANDLE)
      vkDestroyDevice(m_device, nullptr);
}