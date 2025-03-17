#include "Device.h"
#include <Yxis/Logger.h>
#include "../Window.h"

using namespace Yxis::Vulkan;
using QueueFlags = std::bitset<32>;

Device::Device(VkPhysicalDevice physicalDevice)
   : m_physicalDevice(physicalDevice)
{
   constexpr std::array<const char*, 0> deviceEnabledLayers = {};
   constexpr const char* deviceEnabledExtensions[] = { 
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
#ifdef YX_DEBUG
      VK_EXT_DEBUG_MARKER_EXTENSION_NAME
#endif
   };

   // queues
   {
      // fill m_queueFamilies
      uint32_t queueCount;
      vkGetPhysicalDeviceQueueFamilyProperties2(m_physicalDevice, &queueCount, nullptr);
      std::vector<VkQueueFamilyProperties2> queueFamilies(queueCount, VkQueueFamilyProperties2{ VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 });
      vkGetPhysicalDeviceQueueFamilyProperties2(m_physicalDevice, &queueCount, queueFamilies.data());

      auto testQueueFlags = [](const VkQueueFlags flags, const uint32_t bits) { return (flags & bits) == bits; };
      for (uint32_t i = 0; i < queueFamilies.size(); i++)
      {
         // TODO: Video encode, Video decode, possibly sparse resources if ever needed
         const VkQueueFamilyProperties& properties = queueFamilies[i].queueFamilyProperties;
         if (properties.queueCount > 0)
         {
            if (testQueueFlags(properties.queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) // graphics doesn't need to advertise VK_QUEUE_TRANSFER_BIT (link below)
            {
               m_queues.graphics.familyIndex = i;
               m_queues.graphics.queues.resize(1); // at least 1
            }

            // check if this is dedicated compute family
            if (not m_queues.compute.has_value()
               && testQueueFlags(properties.queueFlags, VK_QUEUE_COMPUTE_BIT)
               && not testQueueFlags(properties.queueFlags, VK_QUEUE_GRAPHICS_BIT)
               )
               m_queues.compute = Queue{ .familyIndex = i };

            // dedicated transfer queue
            if (not m_queues.transfer.has_value()
               && testQueueFlags(properties.queueFlags, VK_QUEUE_TRANSFER_BIT)
               && not testQueueFlags(properties.queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)
            )
               m_queues.transfer = Queue{ .familyIndex = i };

            // optical flow queue (maybe for later use)
            if (not m_queues.nvOpticalFlow.has_value()
               && testQueueFlags(properties.queueFlags, VK_QUEUE_OPTICAL_FLOW_BIT_NV)
               && not testQueueFlags(properties.queueFlags, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)
            )
               m_queues.nvOpticalFlow = Queue{ .familyIndex = i };
         }
      }
   }
   
   {
      static constexpr float QUEUE_PRIORITY = 1.0f;
      // https://community.khronos.org/t/question-about-queue-families/108131/2
      // there's always one multi-purpose, and it's going to be the gfx queue
      std::vector<VkDeviceQueueCreateInfo> queuesCreateInfos(1, VkDeviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, m_queues.graphics.familyIndex, 1, &QUEUE_PRIORITY });
      auto enableDedicatedQueue = [&](std::optional<Queue>& optionalQueue, uint32_t howMany) {
         if (optionalQueue.has_value())
         {
            queuesCreateInfos.emplace_back(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, optionalQueue.value().familyIndex, howMany, &QUEUE_PRIORITY);
            optionalQueue.value().queues.resize(howMany);
            return;
         }

         queuesCreateInfos[0].queueCount++;
      };

      enableDedicatedQueue(m_queues.compute, 1);
      enableDedicatedQueue(m_queues.transfer, 1);

      // optical flow queue is a special case and has to be handled manually (it may exist but it's not necessary)
      // only one enabled for now
      if (m_queues.nvOpticalFlow.has_value())
      {
         queuesCreateInfos.emplace_back(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, m_queues.nvOpticalFlow.value().familyIndex, 1, &QUEUE_PRIORITY);
         m_queues.nvOpticalFlow.value().queues.resize(1);
      }

      VkPhysicalDeviceProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, nullptr };
      vkGetPhysicalDeviceProperties2(m_physicalDevice, &properties);

      // linking order (vk14 -> vk13 -> vk12 -> vk11)
      VkPhysicalDeviceVulkan11Features vulkan11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
      VkPhysicalDeviceVulkan12Features vulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &vulkan11Features };
      VkPhysicalDeviceVulkan13Features vulkan13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, &vulkan12Features };
      VkPhysicalDeviceVulkan14Features vulkan14Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, &vulkan13Features };
      VkPhysicalDeviceFeatures2 features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &vulkan14Features };
      vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features);

      VkDeviceCreateInfo deviceCreateInfo =
      {
         .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
         .pNext = &features,
         .flags = 0,
         .queueCreateInfoCount = static_cast<uint32_t>(queuesCreateInfos.size()),
         .pQueueCreateInfos = queuesCreateInfos.data(),
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
   }

   VkDeviceQueueInfo2 queueInfo{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2, .queueFamilyIndex = m_queues.graphics.familyIndex };
   auto getOptionalQueues = [&](std::optional<Queue>& optionalQueue) {
      if (optionalQueue.has_value())
      {
         Queue& value = optionalQueue.value();
         queueInfo.queueFamilyIndex = value.familyIndex;

         for (size_t i = 0; i < value.queues.size(); i++)
         {
            queueInfo.queueIndex = i;
            vkGetDeviceQueue2(m_device, &queueInfo, &value.queues[i]);
         }
      }
   };

   // gfx goes first
   for (size_t i = 0; i < m_queues.graphics.queues.size(); i++)
   {
      queueInfo.queueIndex = static_cast<uint32_t>(i);
      vkGetDeviceQueue2(m_device, &queueInfo, &m_queues.graphics.queues[i]);
   }

   getOptionalQueues(m_queues.compute);
   getOptionalQueues(m_queues.transfer);
   getOptionalQueues(m_queues.nvOpticalFlow);

   m_swapchain = std::make_unique<Swapchain>(this);
}

Device::operator VkDevice() const
{
   return m_device;
}
Device::operator VkPhysicalDevice() const
{
   return m_physicalDevice;
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

const Queues& Device::getDeviceQueues() const
{
   return m_queues;
}

const TimelineSemaphore Device::createTimelineSemaphore() const
{
   return TimelineSemaphore(this, 0);
}

Device::~Device()
{
   m_swapchain.reset();
   if (m_device != VK_NULL_HANDLE)
      vkDestroyDevice(m_device, nullptr);
}