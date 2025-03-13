#include "TimelineSemaphore.h"
#include "Device.h"
#include <Yxis/Logger.h>

using namespace Yxis::Vulkan;

TimelineSemaphore::TimelineSemaphore(const Device* device, const uint64_t initialValue)
   : m_device(device)
{
   const VkSemaphoreTypeCreateInfo semaphoreType =
   {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .pNext = nullptr,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      .initialValue = initialValue,
   };

   const VkSemaphoreCreateInfo createInfo =
   {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &semaphoreType,
      .flags = 0,
   };

   VkResult result = vkCreateSemaphore(*m_device, &createInfo, nullptr, &m_semaphore);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to create semaphore. {}", string_VkResult(result)));
}

void TimelineSemaphore::wait(const uint64_t waitValue, const uint64_t timeout)
{
   const VkSemaphoreWaitInfo waitInfo =
   {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
      .pNext = nullptr,
      .semaphoreCount = 1,
      .pSemaphores = &m_semaphore,
      .pValues = &waitValue,
   };

   VkResult result = vkWaitSemaphores(*m_device, &waitInfo, timeout);
   if (result == VK_TIMEOUT)
   {
      YX_CORE_LOGGER->warn("Reached a timeout while waiting for semaphore.");
      return;
   }
   else if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to wait for a semaphore. {}", string_VkResult(result)));
}

void TimelineSemaphore::signal(const uint64_t value)
{
   const VkSemaphoreSignalInfo signalInfo =
   {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
      .semaphore = m_semaphore,
      .value = value
   };

   VkResult result = vkSignalSemaphore(*m_device, &signalInfo);
   if (result != VK_SUCCESS)
      throw std::runtime_error(fmt::format("Failed to signal a semaphore. {}", string_VkResult(result)));
}

TimelineSemaphore::~TimelineSemaphore()
{
   vkDestroySemaphore(*m_device, m_semaphore, nullptr);
}