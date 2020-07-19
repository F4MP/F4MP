#include "Networking.h"

f4mp::networking::MessageOptions::MessageOptions(bool reliable, Entity* target, Entity* except) : reliable(reliable), target(target), except(except)
{
}

f4mp::networking::Entity::Entity(Networking& networking) : _interface(networking.GetEntityInterface())
{
}

void f4mp::networking::Entity::SendMessage(Event::Type messageType, const EventCallback& callback, const MessageOptions& options)
{
	_interface->SendMessage(messageType, callback, options);
}
