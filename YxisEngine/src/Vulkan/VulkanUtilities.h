#pragma once

namespace Yxis::Vulkan::Utils
{
   // device-related
   VkPhysicalDeviceFeatures2 getDeviceFeatures(VkPhysicalDevice physicalDevice) noexcept;
   VkPhysicalDeviceProperties2 getDeviceProperties(VkPhysicalDevice physicalDevice) noexcept;
   
   // this function is in case some more linked structures were needed
   VkPhysicalDeviceMemoryProperties2 getDeviceMemoryProperties(VkPhysicalDevice physicalDevice) noexcept;
   
   std::vector<VkQueueFamilyProperties2> getDeviceQueueFamilies(VkPhysicalDevice physicalDevice) noexcept;
   bool deviceQueueHasCapabilities(const VkQueueFamilyProperties& properties, VkQueueFlags capabilites) noexcept;
}