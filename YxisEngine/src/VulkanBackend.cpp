#include "VulkanBackend.h"
#include <stdexcept>
#include <spdlog/fmt/fmt.h>
#include <SDL3/SDL_vulkan.h>
#include "vk_enum_string_helper.h"
#include <Yxis/Logger.h>

#ifdef YX_DEBUG
#define YX_DEBUG_LAYERS { "VK_LAYER_KHRONOS_validation" }
#define YX_DEBUG_EXTENSIONS { "VK_EXT_debug_utils", "VK_EXT_layer_settings" }
#else
#define YX_DEBUG_LAYERS {}
#define YX_DEBUG_EXTENSIONS {}
#endif

namespace Yxis::Vulkan
{
   HasExtensionsAndLayers::HasExtensionsAndLayers(const HasExtensionsAndLayers::list_t& requiredLayers, const HasExtensionsAndLayers::list_t& requiredExtensions)
   {
      m_layers = requiredLayers;
      m_extensions = requiredExtensions;
   }

   const HasExtensionsAndLayers::list_t& HasExtensionsAndLayers::getEnabledLayers() const noexcept
   {
      return m_layers;
   }

   const HasExtensionsAndLayers::list_t& HasExtensionsAndLayers::getEnabledExtensions() const noexcept
   {
      return m_extensions;
   }

   void HasExtensionsAndLayers::addLayer(const std::string_view name) noexcept
   {
      return m_layers.push_back(name.data());
   }

   void HasExtensionsAndLayers::addLayers(const list_t& names)
   {
      m_layers.insert(m_layers.end(), names.begin(), names.end());
   }

   void HasExtensionsAndLayers::addLayers(const char** names, const size_t size)
   {
      m_layers.insert(m_layers.end(), names, names + size);
   }

   void HasExtensionsAndLayers::addExtension(const std::string_view name) noexcept
   {
      return m_extensions.push_back(name.data());
   }

   void HasExtensionsAndLayers::addExtensions(const list_t& names)
   {
      m_extensions.insert(m_extensions.end(), names.begin(), names.end());
   }

   void HasExtensionsAndLayers::addExtensions(const char** names, const size_t size)
   {
      m_extensions.insert(m_extensions.end(), names, names + size);
   }

   static VkBool32 debugMessengerCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
      const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
      void*                                        pUserData
   )
   {
      switch (messageSeverity)
      {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
         YX_CORE_LOGGER->warn(pCallbackData->pMessage);
         break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
         YX_CORE_LOGGER->error(pCallbackData->pMessage);
         break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
         YX_CORE_LOGGER->info(pCallbackData->pMessage);
         break;
      default:
         YX_CORE_LOGGER->debug(pCallbackData->pMessage);
      }

      return VK_FALSE;
   }

   static constexpr VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo =
   {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = nullptr,
      .flags = 0,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugMessengerCallback,
      .pUserData = nullptr
   };

   Instance::Instance(const std::string_view applicationName, const AppVersion version)
      : m_applicationName(applicationName), m_applicationVersion(version), HasExtensionsAndLayers(YX_DEBUG_LAYERS, YX_DEBUG_EXTENSIONS)
   {
      const auto& enabledLayers = getEnabledLayers();
      const auto& enabledExtensions = getEnabledExtensions();

      uint32_t surfaceExtensionCount;
      SDL_Vulkan_GetInstanceExtensions(&surfaceExtensionCount);
      addExtensions(const_cast<const char**>(SDL_Vulkan_GetInstanceExtensions(&surfaceExtensionCount)), surfaceExtensionCount); // necessary pain we need to take, we dont modify data inside

      const VkApplicationInfo appInfo =
      {
         .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
         .pNext = nullptr,
         .pApplicationName = m_applicationName.data(),
         .applicationVersion = VK_MAKE_VERSION(m_applicationVersion.major, m_applicationVersion.minor, m_applicationVersion.patch),
         .pEngineName = "Yxis",
         .engineVersion = VK_MAKE_VERSION(1, 0, 0),
         .apiVersion = VK_API_VERSION_1_4,
      };

      const VkInstanceCreateInfo createInfo =
      {
         .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
         #ifdef YX_DEBUG
         .pNext = &debugMessengerCreateInfo,
         #else
         .pNext = nullptr,
         #endif
         .flags = 0,
         .pApplicationInfo = &appInfo,
         .enabledLayerCount = static_cast<uint32_t>(enabledLayers.size()),
         .ppEnabledLayerNames = enabledLayers.data(),
         .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
         .ppEnabledExtensionNames = enabledExtensions.data()
      };

      VkResult res = vkCreateInstance(&createInfo, nullptr, &m_handle);
      if (res != VK_SUCCESS)
      {
         throw std::runtime_error(fmt::format("Failed to create Vulkan instance. {}", string_VkResult(res)));
      }

      volkLoadInstance(m_handle);

      #ifdef YX_DEBUG
      res = vkCreateDebugUtilsMessengerEXT(m_handle, &debugMessengerCreateInfo, nullptr, &m_debugMessengerHandle);
      if (res != VK_SUCCESS)
      {
         throw std::runtime_error(fmt::format("Failed to create Vulkan debug utils messenger. {}", string_VkResult(res)));
      }
      #endif
   }

   Instance::~Instance()
   {
      vkDestroyInstance(m_handle, nullptr);
   }
}