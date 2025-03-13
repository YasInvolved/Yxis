#pragma once

#include "../internal_pch.h"

namespace Yxis::Vulkan
{
   class Device;

   // Timeline semaphore wrapper
   class TimelineSemaphore
   {
   public:
      TimelineSemaphore(const Device* device, const uint64_t initialValue = 0);
      ~TimelineSemaphore();

      void wait(const uint64_t waitValue, const uint64_t timeout = UINT64_MAX);
      void signal(const uint64_t value);
   private:
      VkSemaphore m_semaphore;
      const Device* m_device;
   };
}