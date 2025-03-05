#include "VulkanUtilities.h"

namespace Yxis::Vulkan::Utils
{
   VkPhysicalDeviceProperties2 getDeviceProperties(VkPhysicalDevice physicalDevice) noexcept
   {
      VkPhysicalDeviceProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

      // link structures (vk14 -> vk13 -> vk12 -> vk11)
      VkPhysicalDeviceVulkan11Properties vulkan11Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
      VkPhysicalDeviceVulkan12Properties vulkan12Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, &vulkan11Properties };
      VkPhysicalDeviceVulkan13Properties vulkan13Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES, &vulkan12Properties };
      VkPhysicalDeviceVulkan14Properties vulkan14Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES, &vulkan13Properties };
      properties.pNext = &vulkan14Properties;

      vkGetPhysicalDeviceProperties2(physicalDevice, &properties);

      return properties;
   }

   VkPhysicalDeviceMemoryProperties2 getDeviceMemoryProperties(VkPhysicalDevice physicalDevice) noexcept
   {
      VkPhysicalDeviceMemoryProperties2 memoryProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
      vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memoryProperties);

      return memoryProperties;
   }

   std::vector<VkQueueFamilyProperties2> getDeviceQueueFamilies(VkPhysicalDevice physicalDevice) noexcept
   {
      uint32_t queueFamiliesCount;
      vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamiliesCount, nullptr);
      std::vector<VkQueueFamilyProperties2> queueFamilies(queueFamiliesCount, VkQueueFamilyProperties2{ VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 });
      vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamiliesCount, queueFamilies.data());

      return queueFamilies;
   }

   bool deviceQueueHasCapabilities(const VkQueueFamilyProperties& properties, VkQueueFlags capabilites) noexcept
   {
      if (properties.queueCount > 0 && (properties.queueFlags & capabilites) == capabilites)
         return true;

      return false;
   }
}