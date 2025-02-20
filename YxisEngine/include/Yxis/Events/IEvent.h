#pragma once

#include <Yxis/definitions.h>

namespace Yxis::Events
{
	class YX_API IEvent
	{
	public:
		virtual ~IEvent() = default;
	};
}