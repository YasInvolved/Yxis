#include "Device.h"
#include <Yxis/Logger.h>
#include "../Window.h"

#include "Swapchain.h"

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

   constexpr const char* deviceEnabledLayers[] = { nullptr }; // unintialized yet
   constexpr const char* deviceEnabledExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

   VkDeviceCreateInfo deviceCreateInfo =
   {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
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

   m_swapchain = std::make_unique<Swapchain>(this);
}

const VkDevice Device::getLogicalDevice() const
{
   return m_device;
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

Device::~Device()
{
   m_swapchain.reset();
   if (m_device != VK_NULL_HANDLE)
      vkDestroyDevice(m_device, nullptr);
}