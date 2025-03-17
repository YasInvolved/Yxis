#include "Swapchain.h"
#include "Device.h"
#include "../Window.h"

using namespace Yxis::Vulkan;

Swapchain::Swapchain(const Device* device)
   : m_device(device)
{
   const auto surfaceCapabilites = device->getSurfaceCapabilities().surfaceCapabilities;
   
   VkSurfaceFormatKHR surfaceFormat{ VK_FORMAT_UNDEFINED };
   {
      const auto surfaceFormats = device->getSurfaceFormats();
      for (const auto& [sType, pNext, availableFormat] : surfaceFormats)
      {
         if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
         {
            surfaceFormat = availableFormat;
            break;
         }
      }

      if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
         surfaceFormat = surfaceFormats[0].surfaceFormat;
   }

   VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // fifo is always available
   {
      const auto presentModes = device->getPresentModes();
      for (const auto availablePresentMode : presentModes)
      {
         if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            presentMode = availablePresentMode;
      }
   }

   {
      const auto gfxQueueIndex = m_device->getDeviceQueues().graphics.familyIndex;
      VkSwapchainCreateInfoKHR createInfo =
      {
         .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
         .pNext = nullptr,
         .flags = 0,
         .surface = Window::getSurface(),
         .minImageCount = surfaceCapabilites.minImageCount,
         .imageFormat = surfaceFormat.format,
         .imageColorSpace = surfaceFormat.colorSpace,
         .imageExtent = surfaceCapabilites.currentExtent,
         .imageArrayLayers = 1,
         .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
         .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
         .queueFamilyIndexCount = 1,
         .pQueueFamilyIndices = &gfxQueueIndex,
         .preTransform = surfaceCapabilites.currentTransform,
         .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
         .clipped = VK_FALSE,
         .oldSwapchain = nullptr
      };

      VkResult result = vkCreateSwapchainKHR(m_device->getLogicalDevice(), &createInfo, nullptr, &m_swapchain);
      if (result != VK_SUCCESS)
         throw std::runtime_error(fmt::format("Failed to create swapchain. {}", string_VkResult(result)));
   }

   uint32_t imageCount;
   vkGetSwapchainImagesKHR(m_device->getLogicalDevice(), m_swapchain, &imageCount, nullptr);
   m_swapchainImages.resize(imageCount);
   m_swapchainImageViews.resize(imageCount);
   vkGetSwapchainImagesKHR(m_device->getLogicalDevice(), m_swapchain, &imageCount, m_swapchainImages.data());

   VkImageViewCreateInfo createInfo =
   {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = surfaceFormat.format,
      .components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
      .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
   };

   for (uint32_t i = 0; i < imageCount; i++)
   {
      createInfo.image = m_swapchainImages[i];
      VkResult result = vkCreateImageView(m_device->getLogicalDevice(), &createInfo, nullptr, &m_swapchainImageViews[i]);
      if (result != VK_SUCCESS)
         throw std::runtime_error(fmt::format("Failed to create image view for swapchain image index {}. {}", i, string_VkResult(result)));
   }
}

Swapchain::~Swapchain()
{
   for (const auto imageView : m_swapchainImageViews)
      vkDestroyImageView(m_device->getLogicalDevice(), imageView, nullptr);

   if (m_swapchain != VK_NULL_HANDLE)
      vkDestroySwapchainKHR(m_device->getLogicalDevice(), m_swapchain, nullptr);
}