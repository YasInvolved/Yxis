#include "VulkanRenderer.h"
#include "../Window.h"
#include <Yxis/Logger.h>

using namespace Yxis::Vulkan;

constexpr uint32_t ENGINE_VERSION = VK_MAKE_VERSION(1, 0, 0);

static VkBool32 DebugMessengerCallback(
   VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   VkDebugUtilsMessageTypeFlagsEXT  messageTypes,
   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
   void* pUserData)
{
   switch (messageSeverity)
   {
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      YX_CORE_LOGGER->info(pCallbackData->pMessage);
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      YX_CORE_LOGGER->info(pCallbackData->pMessage);
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      YX_CORE_LOGGER->error(pCallbackData->pMessage);
      break;
   }

   return VK_FALSE;
}

constexpr VkDebugUtilsMessageSeverityFlagsEXT MESSAGE_SEVERITIES = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | 
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

constexpr VkDebugUtilsMessageTypeFlagsEXT MESSAGE_TYPES = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

static constexpr VkDebugUtilsMessengerCreateInfoEXT DEBUG_MESSENGER_CREATE_INFO =
{
   .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
   .pNext = nullptr,
   .flags = 0,
   .messageSeverity = MESSAGE_SEVERITIES,
   .messageType = MESSAGE_TYPES,
   .pfnUserCallback = DebugMessengerCallback,
   .pUserData = nullptr,
};

static void getDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2& features)
{
   features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
   
   // link structures (vk14 -> vk13 -> vk12 -> vk11)
   VkPhysicalDeviceVulkan11Features vulkan11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
   VkPhysicalDeviceVulkan12Features vulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, &vulkan11Features };
   VkPhysicalDeviceVulkan13Features vulkan13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, &vulkan12Features };
   VkPhysicalDeviceVulkan14Features vulkan14Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, &vulkan13Features };
   features.pNext = &vulkan14Features;

   vkGetPhysicalDeviceFeatures2(physicalDevice, &features);
}

static void getDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2& properties)
{
   properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

   // link structures (vk14 -> vk13 -> vk12 -> vk11)
   VkPhysicalDeviceVulkan11Properties vulkan11Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
   VkPhysicalDeviceVulkan12Properties vulkan12Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, &vulkan11Properties };
   VkPhysicalDeviceVulkan13Properties vulkan13Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES, &vulkan12Properties };
   VkPhysicalDeviceVulkan14Properties vulkan14Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES, &vulkan13Properties };
   properties.pNext = &vulkan14Properties;

   vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
}

VulkanRenderer::VulkanRenderer(const std::string& appName)
	: m_appName(appName)
{
   volkInitialize();
   VkResult result;

   {
      auto instanceExtensions = Window::getRequiredInstanceExtensions();
#ifdef YX_DEBUG
      instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      constexpr const char* INSTANCE_ENABLED_LAYERS[] = { "VK_LAYER_KHRONOS_validation" };
#else
      constexpr const char* INSTANCE_ENABLED_LAYERS = {  };
#endif

      const VkApplicationInfo appInfo =
      {
         .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
         .pNext = nullptr,
         .pApplicationName = appName.data(),
         .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
         .pEngineName = "Yxis",
         .engineVersion = ENGINE_VERSION,
         .apiVersion = VK_API_VERSION_1_4,
      };

      const VkInstanceCreateInfo instanceCreateInfo =
      {
         .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef YX_DEBUG
         .pNext = &DEBUG_MESSENGER_CREATE_INFO,
#else
         .pNext = nullptr,
#endif
         .flags = 0,
         .pApplicationInfo = &appInfo,
         .enabledLayerCount = static_cast<uint32_t>(std::size(INSTANCE_ENABLED_LAYERS)),
         .ppEnabledLayerNames = INSTANCE_ENABLED_LAYERS,
         .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
         .ppEnabledExtensionNames = instanceExtensions.data(),
      };

      result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
      if (result != VK_SUCCESS)
      {
         throw std::runtime_error(fmt::format("Couldn't create Vulkan instance. {}", string_VkResult(result)));
      }

      volkLoadInstance(m_instance);

#ifdef YX_DEBUG
      result = vkCreateDebugUtilsMessengerEXT(m_instance, &DEBUG_MESSENGER_CREATE_INFO, nullptr, &m_debugMessenger);
      if (result != VK_SUCCESS)
      {
         throw std::runtime_error(fmt::format("Couldn't create Vulkan debug messenger. {}", string_VkResult(result)));
      }
#endif
   }

   {
      uint32_t devicesCount;
      vkEnumeratePhysicalDevices(m_instance, &devicesCount, nullptr);
      std::vector<VkPhysicalDevice> physicalDevices(devicesCount);
      vkEnumeratePhysicalDevices(m_instance, &devicesCount, physicalDevices.data());

      for (const auto& physicalDevice : physicalDevices)
      {
         VkPhysicalDeviceFeatures2 features;
         getDeviceFeatures(physicalDevice, features);

         VkPhysicalDeviceProperties2 properties;
         getDeviceProperties(physicalDevice, properties);

         VkPhysicalDeviceMemoryProperties2 memoryProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
         vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memoryProperties);

         YX_CORE_LOGGER->info("Found device!");
         YX_CORE_LOGGER->info("Name: {}", properties.properties.deviceName);
         YX_CORE_LOGGER->info("Type: {}", string_VkPhysicalDeviceType(properties.properties.deviceType));
      }
   }
}

VulkanRenderer::~VulkanRenderer()
{
#ifdef YX_DEBUG
   if (m_debugMessenger != VK_NULL_HANDLE)
   {
      vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
   }
#endif
   if (m_instance != VK_NULL_HANDLE)
   {
      vkDestroyInstance(m_instance, nullptr);
   }
}