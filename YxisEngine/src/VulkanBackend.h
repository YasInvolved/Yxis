#pragma once

#include <volk.h>
#include <string>
#include <vector>
#include <array>
#include <optional>

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

      list_t& getEnabledLayers() noexcept;
      list_t& getEnabledExtensions() noexcept;

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

   struct QueueFamilies
   {
      uint32_t gfxQueueIndex;
      std::optional<uint32_t> computeQueueIndex;
      std::optional<uint32_t> transferQueueIndex;
      std::optional<uint32_t> sparseBindingQueueIndex;
   };

   static constexpr uint32_t GFX_QUEUES_COUNT = 1;
   static constexpr uint32_t COMPUTE_QUEUES_COUNT = 1;
   static constexpr uint32_t TRANSFER_QUEUES_COUNT = 1;
   static constexpr uint32_t SPARSE_QUEUES_COUNT = 1;

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
      VkDevice getHandle() const noexcept;
      VkPhysicalDevice getPhysicalHandle() const noexcept;
      const QueueFamilies& getQueueFamilies() const noexcept;
   private:
      VkDevice m_handle = VK_NULL_HANDLE;
      VkPhysicalDevice m_physicalHandle;
      VkPhysicalDeviceMemoryProperties m_memoryProperties;
      VkPhysicalDeviceProperties m_deviceProperties;
      VkPhysicalDeviceFeatures m_deviceAvailableFeatures;
      VkPhysicalDeviceFeatures m_deviceEnabledFeatures;
      std::vector<VkPresentModeKHR> m_availablePresentModes;
      QueueFamilies m_queueFamilies;
      std::array<VkQueue, GFX_QUEUES_COUNT> m_graphicsQueues = {};
      std::array<VkQueue, COMPUTE_QUEUES_COUNT> m_computeQueues = {};
      std::array<VkQueue, TRANSFER_QUEUES_COUNT> m_transferQueues = {};
      std::array<VkQueue, SPARSE_QUEUES_COUNT> m_sparseQueues = {};
   };

   class Instance : public HasExtensionsAndLayers
   {
   public:
      using devicelist_t = std::vector<Device>;

      Instance(const std::string_view applicationName, const AppVersion version);
      ~Instance();

      void initialize() override;

      VkInstance getHandle() const;
      devicelist_t getDevices() const;
      Device getBestDevice() const;
   private:
      const std::string_view m_applicationName;
      const AppVersion m_applicationVersion;
      VkInstance m_handle = VK_NULL_HANDLE;
      VkSurfaceKHR m_surface = VK_NULL_HANDLE;
      VkDebugUtilsMessengerEXT m_debugMessengerHandle = VK_NULL_HANDLE;
   };

   class Swapchain
   {
   public:
      Swapchain(const Device& device);

      void create();

      ~Swapchain();
   private:
      const Device& m_device;
      VkSwapchainKHR m_handle = VK_NULL_HANDLE;
      VkFormat m_swapchainFormat = VK_FORMAT_MAX_ENUM;
      VkColorSpaceKHR m_swapchainColorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
      std::vector<VkImage> m_swapchainImages;
      std::vector<VkImageView> m_swapchainImageViews;
   };
}