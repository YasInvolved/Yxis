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
      uint32_t surfaceExtensionCount;
      SDL_Vulkan_GetInstanceExtensions(&surfaceExtensionCount);
      addExtensions(const_cast<const char**>(SDL_Vulkan_GetInstanceExtensions(&surfaceExtensionCount)), surfaceExtensionCount); // necessary pain we need to take, we dont modify data inside
   }

   void Instance::initialize()
   {
      const auto& enabledLayers = getEnabledLayers();
      const auto& enabledExtensions = getEnabledExtensions();

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

   Instance::devicelist_t Instance::getDevices() const
   {
      assert(m_handle != VK_NULL_HANDLE && "Vulkan::Instance m_handle is null");

      uint32_t devicesCount;
      vkEnumeratePhysicalDevices(m_handle, &devicesCount, nullptr);
      VkPhysicalDevice* rawDevices = new VkPhysicalDevice[devicesCount];
      vkEnumeratePhysicalDevices(m_handle, &devicesCount, rawDevices);

      std::vector<Device> devices;
      for (uint32_t i = 0; i < devicesCount; i++)
      {
         devices.emplace_back(rawDevices[i]);
      }

      delete[] rawDevices;
      return devices;
   }

   Device Instance::getBestDevice() const
   {
      auto devices = getDevices();
      if (devices.size() == 1)
         return devices[0];

      uint32_t bestRating = 0;
      size_t bestDeviceIndex = 0;
      for (size_t i = 0; i < devices.size(); i++)
      {
         const auto& device = devices[i];
         uint32_t rating = 0;

         const auto& properties = device.getDeviceProperties();
         const auto& features = device.getAvailableDeviceFeatures();

         switch (properties.deviceType)
         {
         case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
         case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            rating += 100;
            break;
         case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            rating += 80;
         }

         if (features.tessellationShader)
            rating += 50;
         if (features.geometryShader)
            rating += 50;
         
         if (rating > bestRating)
         {
            bestRating = rating;
            bestDeviceIndex = i;
         }
      }

      return devices[bestDeviceIndex];
   }

   Instance::~Instance()
   {
      vkDestroyInstance(m_handle, nullptr);
   }

   Device::Device(VkPhysicalDevice physicalHandle)
      : m_physicalHandle(physicalHandle), HasExtensionsAndLayers()
   {
      vkGetPhysicalDeviceFeatures(m_physicalHandle, &m_deviceAvailableFeatures);
      vkGetPhysicalDeviceProperties(m_physicalHandle, &m_deviceProperties);
      vkGetPhysicalDeviceMemoryProperties(m_physicalHandle, &m_memoryProperties);
      m_deviceEnabledFeatures = m_deviceAvailableFeatures; // enable all available features by default
   }

   void Device::initialize()
   {

   }

   const VkPhysicalDeviceMemoryProperties& Device::getMemoryProperties() const noexcept
   {
      return m_memoryProperties;
   }

   const VkPhysicalDeviceProperties& Device::getDeviceProperties() const noexcept
   {
      return m_deviceProperties;
   }

   const VkPhysicalDeviceFeatures& Device::getAvailableDeviceFeatures() const noexcept
   {
      return m_deviceAvailableFeatures;
   }

   VkPhysicalDeviceFeatures& Device::getEnabledDeviceFeatures() noexcept
   {
      return m_deviceEnabledFeatures;
   }

   Device::~Device()
   {
      if (m_handle != VK_NULL_HANDLE)
         vkDestroyDevice(m_handle, nullptr);
   }
}