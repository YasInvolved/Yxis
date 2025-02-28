#pragma once

namespace Yxis::Vulkan
{
   class Device;

   class Swapchain
   {
   public:
      Swapchain(const Device* device);
      ~Swapchain();
   private:
      const Device* m_device;
      VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
      std::vector<VkImage> m_swapchainImages;
      std::vector<VkImageView> m_swapchainImageViews;
   };
}