#pragma once

#include "Device.h"

namespace Yxis::Vulkan
{
	class VulkanRenderer
	{
	public:
		VulkanRenderer(const std::string& appName);
		~VulkanRenderer();
	private:
		const std::string m_appName;
		
		VkInstance m_instance = VK_NULL_HANDLE;
#ifdef YX_DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif
		std::unique_ptr<Device> m_device;
	};
}