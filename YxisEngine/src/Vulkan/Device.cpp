#include "Device.h"
#include <Yxis/Logger.h>

using namespace Yxis::Vulkan;

Device::Device(VkPhysicalDevice physicalDevice, QueueFamilyIndices&& queueIndices)
   : m_physicalDevice(physicalDevice), m_queueFamilies(std::move(queueIndices))
{
   constexpr float queuePriority = 1.0f; // TODO: make queue priority matter
   std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
   queueCreateInfos.emplace_back(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, m_queueFamilies.gfxIndex, 1, &queuePriority);

   if (m_queueFamilies.computeIndex.has_value())
      queueCreateInfos.emplace_back(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, m_queueFamilies.computeIndex.value(), 1, &queuePriority);
   
   if (m_queueFamilies.transferIndex.has_value())
      queueCreateInfos.emplace_back(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, m_queueFamilies.transferIndex.value(), 1, &queuePriority);

   VkDeviceCreateInfo deviceCreateInfo =
   {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = 0,
      .ppEnabledExtensionNames = nullptr,
   };

   VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
   if (result != VK_SUCCESS)
   {
      throw std::runtime_error(fmt::format("Failed to create logical device. {}", string_VkResult(result)));
   }

   volkLoadDevice(m_device);

   VkDeviceQueueInfo2 queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2, nullptr, 0, m_queueFamilies.gfxIndex, 0 }; // graphics goes first

   vkGetDeviceQueue2(m_device, &queueInfo, &m_queues.graphicsQueue);
   if (m_queueFamilies.computeIndex.has_value())
   {
      assert(m_queues.computeQueue == VK_NULL_HANDLE && "The field of compute queue has been initialized already!");
      queueInfo.queueFamilyIndex = m_queueFamilies.computeIndex.value();
      vkGetDeviceQueue2(m_device, &queueInfo, &m_queues.computeQueue);
   }

   if (m_queueFamilies.transferIndex.has_value())
   {
      assert(m_queues.transferQueue == VK_NULL_HANDLE && "The field of transfer queue has been initialized already!");
      queueInfo.queueFamilyIndex = m_queueFamilies.transferIndex.value();
      vkGetDeviceQueue2(m_device, &queueInfo, &m_queues.transferQueue);
   }
}

Device::~Device()
{
   if (m_device != VK_NULL_HANDLE)
      vkDestroyDevice(m_device, nullptr);
}