#pragma once

#include <Yxis/Events/IEvent.h>

namespace Yxis::Events
{
	class IWindowResizedEvent : public IEvent
	{
	public:
		IWindowResizedEvent(uint32_t newWidth, uint32_t newHeight) 
			: newWidth(newWidth), newHeight(newHeight) {}
		virtual ~IWindowResizedEvent() = default;

		uint32_t newWidth;
		uint32_t newHeight;
	};
}