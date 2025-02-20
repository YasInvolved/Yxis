#pragma once

#include <Yxis/Events/IEvent.h>

namespace Yxis::Events
{
	class YX_API IKeyboardEvent : public IEvent
	{
	public:
		IKeyboardEvent(bool down, uint32_t key, uint32_t mod)
			: down(down), key(key), mod(mod) { }
		virtual ~IKeyboardEvent() = default;
		
		bool down;
		uint32_t key;
		uint32_t mod;
	};
}