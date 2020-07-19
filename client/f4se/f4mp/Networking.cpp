#include "Networking.h"

f4mp::networking::MessageOptions::MessageOptions(bool reliable, Entity* target, Entity* except) : reliable(reliable), target(target), except(except)
{
}

void f4mp::networking::Entity::SendMessage(Event::Type messageType, const EventCallback& callback, const MessageOptions& options)
{
	_interface->SendMessage(messageType, callback, options);
}
