#pragma once

#include <Yxis/definitions.h>
#include <Yxis/Events/IEvent.h>
#include <functional>
#include <unordered_map>
#include <memory>
#include <typeindex>

namespace Yxis::Events
{
	class YX_API EventDispatcher
	{
	public:
		using Handler = std::function<void(const std::shared_ptr<IEvent>&)>;

		template <typename EventType>
		static void subscribe(Handler handler)
		{
			m_handlers[typeid(EventType)].emplace_back(handler);
		}

		static void dispatch(const std::shared_ptr<IEvent>& event);
	private:
		static std::unordered_map<std::type_index, std::vector<Handler>> m_handlers;
	};
}