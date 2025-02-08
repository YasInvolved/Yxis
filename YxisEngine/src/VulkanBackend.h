#pragma once

#include <volk.h>
#include <string>
#include <vector>

namespace Yxis::Vulkan 
{
   class HasExtensionsAndLayers
   {
   public:
      using list_t = std::vector<const char*>;

      HasExtensionsAndLayers(const list_t& requiredLayers = {}, const list_t& requiredExtensions = {});
      virtual ~HasExtensionsAndLayers() = default;

      void addLayer(const std::string_view name) noexcept;
      void addLayers(const list_t& names);
      void addLayers(const char** names, const size_t size);

      void addExtension(const std::string_view name) noexcept;
      void addExtensions(const list_t& names);
      void addExtensions(const char** names, const size_t size);

      const list_t& getEnabledLayers() const noexcept;
      const list_t& getEnabledExtensions() const noexcept;

      virtual void initialize() = 0;
   private:
      list_t m_extensions;
      list_t m_layers;
   };

   struct AppVersion
   {
      uint32_t major;
      uint32_t minor;
      uint32_t patch;
   };

   class Device : public HasExtensionsAndLayers
   {
   public:
      Device(VkPhysicalDevice physicalHandle);
      ~Device();

      void initialize() override;
      const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const noexcept;
      const VkPhysicalDeviceProperties& getDeviceProperties() const noexcept;
      const VkPhysicalDeviceFeatures& getAvailableDeviceFeatures() const noexcept;
      VkPhysicalDeviceFeatures& getEnabledDeviceFeatures() noexcept;
   private:
      VkDevice m_handle = VK_NULL_HANDLE;
      VkPhysicalDevice m_physicalHandle;
      VkPhysicalDeviceMemoryProperties m_memoryProperties;
      VkPhysicalDeviceProperties m_deviceProperties;
      VkPhysicalDeviceFeatures m_deviceAvailableFeatures;
      VkPhysicalDeviceFeatures m_deviceEnabledFeatures;
   };

   class Instance : public HasExtensionsAndLayers
   {
   public:
      using devicelist_t = std::vector<Device>;

      Instance(const std::string_view applicationName, const AppVersion version);
      ~Instance();

      void initialize() override;

      devicelist_t getDevices() const;
      Device getBestDevice() const;
   private:
      const std::string_view m_applicationName;
      const AppVersion m_applicationVersion;
      VkInstance m_handle = VK_NULL_HANDLE;
      VkDebugUtilsMessengerEXT m_debugMessengerHandle = VK_NULL_HANDLE;
   };
}