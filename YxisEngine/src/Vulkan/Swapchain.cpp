#include "Swapchain.h"
#include "Device.h"
#include "../Window.h"
#include <Yxis/Logger.h>

using namespace Yxis::Vulkan;

Swapchain::Swapchain(const Device* device)
   : m_device(device)
{
   const auto [sType, pNext, surfaceCapabilities] = m_device->getSurfaceCapabilities();
   const auto surfaceFormats = m_device->getSurfaceFormats();
   const uint32_t gfxQueueIndex = m_device->getQueueFamilyIndices().gfxIndex;

   VkFormat format = VK_FORMAT_UNDEFINED;
   VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
   for (const auto& [sType, pNext, surfaceFormat] : surfaceFormats)
   {
      YX_CORE_LOGGER->info("Format found: {}", string_VkFormat(surfaceFormat.format));
      if (surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB)
      {
         format = surfaceFormat.format;
         colorSpace = surfaceFormat.colorSpace;
      }
   }

   if (format == VK_FORMAT_UNDEFINED && colorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR)
   {
      format = surfaceFormats[0].surfaceFormat.format;
      colorSpace = surfaceFormats[0].surfaceFormat.colorSpace;
   }

   const auto availablePresentModes = m_device->getPresentModes();
   VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
   for (const auto& availablePresentMode : availablePresentModes)
   {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
         presentMode = availablePresentMode;
   }

   if (presentMode == VK_PRESENT_MODE_MAX_ENUM_KHR)
   {
      presentMode = availablePresentModes[0];
   }

   const VkSwapchainCreateInfoKHR createInfo =
   {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .surface = Window::getSurface(),
      .minImageCount = surfaceCapabilities.minImageCount,
      .imageFormat = format,
      .imageColorSpace = colorSpace,
      .imageExtent = surfaceCapabilities.currentExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &gfxQueueIndex,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_FALSE,
      .oldSwapchain = nullptr
   };

   VkResult result = vkCreateSwapchainKHR(m_device->getLogicalDevice(), &createInfo, nullptr, &m_swapchain);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create swapchain. {}", string_VkResult(result)));

   uint32_t imagesCount;
   vkGetSwapchainImagesKHR(m_device->getLogicalDevice(), m_swapchain, &imagesCount, nullptr);
   m_swapchainImages.resize(imagesCount);
   m_swapchainImageViews.resize(imagesCount);
   result = vkGetSwapchainImagesKHR(m_device->getLogicalDevice(), m_swapchain, &imagesCount, m_swapchainImages.data());
   if (result != VK_SUCCESS)
   {
      vkDestroySwapchainKHR(m_device->getLogicalDevice(), m_swapchain, nullptr);
      throw std::runtime_error(fmt::format("Failed to acquire swapchain images. {}", string_VkResult(result)));
   }
   
   for (size_t i = 0; i < imagesCount; i++)
   {
      VkImageViewCreateInfo createInfo =
      {
         .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
         .pNext = nullptr,
         .flags = 0,
         .image = m_swapchainImages[i],
         .viewType = VK_IMAGE_VIEW_TYPE_2D,
         .format = format,
         .components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
         .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
      };

      result = vkCreateImageView(m_device->getLogicalDevice(), &createInfo, nullptr, &m_swapchainImageViews[i]);
      if (result != VK_SUCCESS)
      {
         vkDestroySwapchainKHR(m_device->getLogicalDevice(), m_swapchain, nullptr);
         throw std::runtime_error(fmt::format("Failed to create image view for index {}. {}", i, string_VkResult(result)));
      }
   }
}

Swapchain::~Swapchain()
{
   for (const auto imageView : m_swapchainImageViews)
   {
      vkDestroyImageView(m_device->getLogicalDevice(), imageView, nullptr);
   }

   if (m_swapchain != VK_NULL_HANDLE)
      vkDestroySwapchainKHR(m_device->getLogicalDevice(), m_swapchain, nullptr);
}