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

   class Instance : public HasExtensionsAndLayers
   {
   public:
      Instance(const std::string_view applicationName, const AppVersion version);
      ~Instance();
   private:
      const std::string_view m_applicationName;
      const AppVersion m_applicationVersion;
      VkInstance m_handle = VK_NULL_HANDLE;
      VkDebugUtilsMessengerEXT m_debugMessengerHandle = VK_NULL_HANDLE;
   };

   class Device : public HasExtensionsAndLayers
   {
   };
}