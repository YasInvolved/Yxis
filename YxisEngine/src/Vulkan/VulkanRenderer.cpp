#include "VulkanRenderer.h"
#include "../Window.h"
#include <Yxis/Logger.h>
#include "VulkanUtilities.h"

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
         VkPhysicalDeviceProperties2 properties = Utils::getDeviceProperties(physicalDevice);

         YX_CORE_LOGGER->info("Found device!");
         YX_CORE_LOGGER->info("Name: {}", properties.properties.deviceName);
         YX_CORE_LOGGER->info("Type: {}", string_VkPhysicalDeviceType(properties.properties.deviceType));

         // discrete gpu is likely the most powerful, i leave it like this just for now
         if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            m_physical = physicalDevice;
      }

      if (m_physical == VK_NULL_HANDLE)
         m_physical = physicalDevices[0];
   }

   {
      const std::vector<VkQueueFamilyProperties2> queueFamilies = std::move(Utils::getDeviceQueueFamilies(m_physical));

      uint32_t gfxQueueIndex = 0;
      std::optional<uint32_t> computeQueueIndex;
      std::optional<uint32_t> transferQueueIndex;

      for (uint32_t i = 0; i < queueFamilies.size(); i++)
      {
         const auto& properties = queueFamilies[i].queueFamilyProperties;
         
         if (Utils::deviceQueueHasCapabilities(properties, VK_QUEUE_GRAPHICS_BIT))
         {
            gfxQueueIndex = i;
            continue;
         }

         if (not computeQueueIndex.has_value() && Utils::deviceQueueHasCapabilities(properties, VK_QUEUE_COMPUTE_BIT))
         {
            computeQueueIndex = i;
            continue;
         }

         if (not transferQueueIndex.has_value() && Utils::deviceQueueHasCapabilities(properties, VK_QUEUE_TRANSFER_BIT))
         {
            transferQueueIndex = i;
            continue;
         }
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