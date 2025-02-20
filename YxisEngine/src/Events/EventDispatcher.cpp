#include <Yxis/Events/EventDispatcher.h>

using namespace Yxis::Events;
using Handler = EventDispatcher::Handler;

std::unordered_map<std::type_index, std::vector<Handler>> EventDispatcher::m_handlers;

void EventDispatcher::dispatch(const std::shared_ptr<IEvent>& event)
{
	auto e = m_handlers.find(typeid(*event));
	if (e != m_handlers.end())
	{
		for (auto& handler : e->second)
			handler(event);
	}
}