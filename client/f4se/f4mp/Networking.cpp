#include "Networking.h"

void f4mp::networking::Entity::SendMessage(Event& event, Event::Type messageType, const EventCallback& callback)
{
	event.GetNetworking().SendMessage(*this, messageType, callback);
}
