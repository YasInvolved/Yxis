#include "VulkanBackend.h"
#include <stdexcept>
#include <spdlog/fmt/fmt.h>
#include <SDL3/SDL_vulkan.h>
#include "vk_enum_string_helper.h"
#include <Yxis/Logger.h>
#include "Window.h"
#include <algorithm>

#ifdef YX_DEBUG
#define YX_DEBUG_LAYERS { "VK_LAYER_KHRONOS_validation" }
#define YX_DEBUG_EXTENSIONS { "VK_EXT_debug_utils" }
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

   HasExtensionsAndLayers::list_t& HasExtensionsAndLayers::getEnabledLayers() noexcept
   {
      return m_layers;
   }

   HasExtensionsAndLayers::list_t& HasExtensionsAndLayers::getEnabledExtensions() noexcept
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
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
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
      auto& enabledLayers = getEnabledLayers();
      auto& enabledExtensions = getEnabledExtensions();

      {
         uint32_t availableLayersCount;
         vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr);
         std::vector<VkLayerProperties> availableLayers(availableLayersCount);
         vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());

         uint32_t availableExtensionCount;
         vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
         std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
         vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

         for (uint32_t i = 0; i < enabledLayers.size(); i++)
         {
            bool found = false;
            for (const auto& availableLayer : availableLayers)
            {
               if (std::strcmp(availableLayer.layerName, enabledLayers[i]) == 0) found = true;
            }

            if (not found)
            {
               YX_CORE_LOGGER->warn("Disabling unavailable instance layer {}", enabledLayers[i]);
               enabledLayers.erase(enabledLayers.begin() + i);
            }
         }

         for (uint32_t i = 0; i < enabledExtensions.size(); i++)
         {
            bool found = false;
            for (const auto& availableExtension : availableExtensions)
            {
               if (std::strcmp(availableExtension.extensionName, enabledExtensions[i]) == 0) found = true;
            }

            if (not found)
            {
               YX_CORE_LOGGER->warn("Disabling unavailable instance extension {}", enabledExtensions[i]);
               enabledExtensions.erase(enabledExtensions.begin() + i);
            }
         }
      }

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
         // debug messenger is optional we can continue without it
         YX_CORE_LOGGER->error("Vulkan Debug Messenger is not available. Vulkan message: {}", string_VkResult(res));
      }
#endif
   }

   VkInstance Instance::getHandle() const
   {
      return m_handle;
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
      Window::destroySurface(m_handle);
      if (m_debugMessengerHandle != VK_NULL_HANDLE)
         vkDestroyDebugUtilsMessengerEXT(m_handle, m_debugMessengerHandle, nullptr);
      vkDestroyInstance(m_handle, nullptr);
   }

   Device::Device(VkPhysicalDevice physicalHandle)
      : m_physicalHandle(physicalHandle), HasExtensionsAndLayers()
   {
      auto surface = Window::getSurface();
      vkGetPhysicalDeviceFeatures(m_physicalHandle, &m_deviceAvailableFeatures);
      vkGetPhysicalDeviceProperties(m_physicalHandle, &m_deviceProperties);
      vkGetPhysicalDeviceMemoryProperties(m_physicalHandle, &m_memoryProperties);
      m_deviceEnabledFeatures = m_deviceAvailableFeatures; // enable all available features by default

      uint32_t queuefamiliesCount;
      vkGetPhysicalDeviceQueueFamilyProperties(m_physicalHandle, &queuefamiliesCount, nullptr);
      std::vector<VkQueueFamilyProperties> queueFamilies(queuefamiliesCount);
      vkGetPhysicalDeviceQueueFamilyProperties(m_physicalHandle, &queuefamiliesCount, queueFamilies.data());

      for (uint32_t i = 0; i < queuefamiliesCount; i++)
      {
         const auto& queueFamily = queueFamilies[i];
         if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
         {
            VkBool32 isSurfaceSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalHandle, i, surface, &isSurfaceSupported);
            if (isSurfaceSupported)
               m_queueFamilies.gfxQueueIndex = i;
         }
         if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
            m_queueFamilies.computeQueueIndex = i;
         if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT))
            m_queueFamilies.transferQueueIndex = i;
         //if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT))
         //   m_queueFamilies.sparseBindingQueueIndex = i;
      }

      {
         uint32_t presentModesCount;
         vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalHandle, surface, &presentModesCount, nullptr);
         m_availablePresentModes.resize(presentModesCount);
         vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalHandle, surface, &presentModesCount, m_availablePresentModes.data());
      }

      addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
   }

   void Device::initialize()
   {
      auto& enabledLayers = getEnabledLayers();
      auto& enabledExtensions = getEnabledExtensions();

      {
         uint32_t availableLayersCount;
         vkEnumerateDeviceLayerProperties(m_physicalHandle, &availableLayersCount, nullptr);
         std::vector<VkLayerProperties> availableLayers(availableLayersCount);
         vkEnumerateDeviceLayerProperties(m_physicalHandle, &availableLayersCount, availableLayers.data());

         uint32_t availableExtensionsCount;
         vkEnumerateDeviceExtensionProperties(m_physicalHandle, nullptr, &availableExtensionsCount, nullptr);
         std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
         vkEnumerateDeviceExtensionProperties(m_physicalHandle, nullptr, &availableExtensionsCount, availableExtensions.data());

         for (uint32_t i = 0; i < enabledLayers.size(); i++)
         {
            bool found = false;
            for (const auto& availableLayer : availableLayers)
            {
               if (std::strcmp(availableLayer.layerName, enabledLayers[i]) == 0) found = true;
            }

            if (not found)
            {
               YX_CORE_LOGGER->warn("Disabling unavailable device layer {}", enabledLayers[i]);
               enabledLayers.erase(enabledLayers.begin() + i);
            }
         }

         for (uint32_t i = 0; i < enabledExtensions.size(); i++)
         {
            bool found = false;
            for (const auto& availableExtension : availableExtensions)
            {
               if (std::strcmp(availableExtension.extensionName, enabledExtensions[i]) == 0) found = true;
            }

            if (not found)
            {
               YX_CORE_LOGGER->warn("Disabling unavailable device extension {}", enabledExtensions[i]);
               enabledExtensions.erase(enabledExtensions.begin() + i);
            }
         }
      }

      {
         constexpr float queuePriority = 1.0f;

         const std::array<VkDeviceQueueCreateInfo, 3> queueCreateInfos =
         {
            {
               { // gfx
                  .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                  .pNext = nullptr,
                  .flags = 0,
                  .queueFamilyIndex = m_queueFamilies.gfxQueueIndex,
                  .queueCount = GFX_QUEUES_COUNT,
                  .pQueuePriorities = &queuePriority,
               },
               { // compute
                  .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                  .pNext = nullptr,
                  .flags = 0,
                  .queueFamilyIndex = m_queueFamilies.computeQueueIndex.value(),
                  .queueCount = COMPUTE_QUEUES_COUNT,
                  .pQueuePriorities = &queuePriority,
               },
               { // transfer
                  .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                  .pNext = nullptr,
                  .flags = 0,
                  .queueFamilyIndex = m_queueFamilies.transferQueueIndex.value(),
                  .queueCount = TRANSFER_QUEUES_COUNT,
                  .pQueuePriorities = &queuePriority
               },
               //{ // sparse binding
               //   .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
               //   .pNext = nullptr,
               //   .flags = 0,
               //   .queueFamilyIndex = m_queueFamilies.sparseBindingQueueIndex.value(),
               //   .queueCount = SPARSE_QUEUES_COUNT,
               //   .pQueuePriorities = &queuePriority
               //}
            }
         };

         const VkDeviceCreateInfo deviceCreateInfo =
         {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = static_cast<uint32_t>(enabledLayers.size()),
            .ppEnabledLayerNames = enabledLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
            .ppEnabledExtensionNames = enabledExtensions.data()
         };

         VkResult res = vkCreateDevice(m_physicalHandle, &deviceCreateInfo, nullptr, &m_handle);
         if (res != VK_SUCCESS)
         {
            throw std::runtime_error("Failed to create a Vulkan device");
         }

         volkLoadDevice(m_handle);
      }

      for (uint32_t i = 0; i < GFX_QUEUES_COUNT; i++)
      {
         vkGetDeviceQueue(m_handle, m_queueFamilies.gfxQueueIndex, i, m_graphicsQueues.data() + i);
      }

      if (m_queueFamilies.computeQueueIndex.has_value()) 
      {
         for (uint32_t i = 0; i < COMPUTE_QUEUES_COUNT; i++)
         {
            vkGetDeviceQueue(m_handle, m_queueFamilies.computeQueueIndex.value(), i, m_computeQueues.data() + i);
         }
      }

      if (m_queueFamilies.transferQueueIndex.has_value())
      {
         for (uint32_t i = 0; i < TRANSFER_QUEUES_COUNT; i++)
         {
            vkGetDeviceQueue(m_handle, m_queueFamilies.transferQueueIndex.value(), i, m_transferQueues.data() + i);
         }
      }

      if (m_queueFamilies.sparseBindingQueueIndex.has_value())
      {
         for (uint32_t i = 0; i < SPARSE_QUEUES_COUNT; i++)
         {
            vkGetDeviceQueue(m_handle, m_queueFamilies.sparseBindingQueueIndex.value(), i, m_sparseQueues.data() + i);
         }
      }
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

   VkDevice Device::getHandle() const noexcept
   {
      return m_handle;
   }

   VkPhysicalDevice Device::getPhysicalHandle() const noexcept
   {
      return m_physicalHandle;
   }

   const QueueFamilies& Device::getQueueFamilies() const noexcept
   {
      return m_queueFamilies;
   }

   Device::~Device()
   {
      if (m_handle != VK_NULL_HANDLE)
         vkDestroyDevice(m_handle, nullptr);
   }

   Swapchain::Swapchain(const Device& device)
      : m_device(device)
   {

   }

   void Swapchain::create()
   {
      {
         const VkPhysicalDevice physicalDevice = m_device.getPhysicalHandle();
         const VkSurfaceCapabilitiesKHR surfaceCapabilities = Window::getSurfaceCapabilities(physicalDevice);
         const Window::surfaceformats_t surfaceFormats = Window::getAvailableSurfaceFormats(physicalDevice);
         const Window::presentmodes_t presentModes = Window::getAvailableSurfacePresentModes(physicalDevice);

         uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
         if (surfaceCapabilities.maxImageCount > imageCount)
         {
            imageCount = std::clamp<uint32_t>(imageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
         }

         auto chooseFormat = [](const Window::surfaceformats_t& formats) -> const VkSurfaceFormatKHR& {
            for (const auto& format : formats)
            {
               if (format.format == VK_FORMAT_R8G8B8A8_SRGB &&
                  format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
               {
                  return format;
               }
            }

            return formats[0]; // return first position if desired format wasn't found
         };

         auto choosePresentMode = [](const Window::presentmodes_t& presentModes) -> const VkPresentModeKHR {
            for (const auto& presentMode : presentModes)
            {
               if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) return presentMode;
            }

            return VK_PRESENT_MODE_FIFO_KHR; // FIFO is always supported
         };

         auto& [format, colorSpace] = chooseFormat(surfaceFormats);
         m_swapchainFormat = format;
         m_swapchainColorSpace = colorSpace;

         const VkSwapchainCreateInfoKHR createInfo =
         {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = Window::getSurface(),
            .minImageCount = imageCount,
            .imageFormat = m_swapchainFormat,
            .imageColorSpace = m_swapchainColorSpace,
            .imageExtent = surfaceCapabilities.currentExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &m_device.getQueueFamilies().gfxQueueIndex,
            .preTransform = surfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = choosePresentMode(presentModes),
            .clipped = VK_FALSE,
            .oldSwapchain = VK_NULL_HANDLE
         };

         VkResult res = vkCreateSwapchainKHR(m_device.getHandle(), &createInfo, nullptr, &m_handle);
         if (res != VK_SUCCESS)
         {
            throw std::runtime_error(fmt::format("Failed to create swapchain. {}", string_VkResult(res)));
         }
      }

      {
         uint32_t imageCount;
         vkGetSwapchainImagesKHR(m_device.getHandle(), m_handle, &imageCount, nullptr);
         m_swapchainImages.resize(imageCount);
         m_swapchainImageViews.resize(imageCount);
         VkResult res = vkGetSwapchainImagesKHR(m_device.getHandle(), m_handle, &imageCount, m_swapchainImages.data());
         if (res != VK_SUCCESS)
         {
            throw std::runtime_error(fmt::format("Failed to acquire swapchain images. {}", string_VkResult(res)));
         }
      }

      {
         for (size_t i = 0; i < m_swapchainImages.size(); i++)
         {
            const VkImageViewCreateInfo createInfo =
            {
               .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
               .pNext = nullptr,
               .flags = 0,
               .image = m_swapchainImages[i],
               .viewType = VK_IMAGE_VIEW_TYPE_2D,
               .format = m_swapchainFormat,
               .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY },
               .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
            };

            VkResult res = vkCreateImageView(m_device.getHandle(), &createInfo, nullptr, &m_swapchainImageViews[i]);
            if (res != VK_SUCCESS)
            {
               throw std::runtime_error(fmt::format("Failed to create image view no. {}. {}", i + 1, string_VkResult(res)));
            }
         }
      }
   }

   Swapchain::~Swapchain()
   {
      const auto device = m_device.getHandle();

      if (m_handle != VK_NULL_HANDLE)
      {
         vkDestroySwapchainKHR(device, m_handle, nullptr);

         for (const auto& imageView : m_swapchainImageViews)
         {
            vkDestroyImageView(device, imageView, nullptr);
         }
      }
   }
}