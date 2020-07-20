#include "Networking.h"

#include <algorithm>
#include <stdexcept>

f4mp::networking::MessageOptions::MessageOptions(bool reliable, Entity* target, Entity* except) : reliable(reliable), target(target), except(except)
{
}

void f4mp::networking::Entity::SendMessage(Event::Type messageType, const EventCallback& callback, const MessageOptions& options)
{
	_interface->SendMessage(messageType, callback, options);
}

void f4mp::networking::Networking::Create(Entity* entity)
{
	if (entity->_interface != nullptr)
	{
		throw std::runtime_error("entity already created.");
	}

	entity->_interface = GetEntityInterface();

	Entity::InstantiationID instantiationID = 0;

	for (auto& entityInstantiation : entityInstantiationQueue)
	{
		instantiationID = std::max<Entity::InstantiationID>(instantiationID, entityInstantiation.first + 1);
	}

	entityInstantiationQueue[instantiationID] = entity;
}

f4mp::networking::Entity* f4mp::networking::Networking::Instantiate(Entity::InstantiationID instantiationID, Entity::ID entityID, Entity::Type entityType)
{
	Entity* entity = nullptr;

	if (instantiationID >= 0)
	{
		auto entityInstantiation = entityInstantiationQueue.find(instantiationID);
		if (entityInstantiation == entityInstantiationQueue.end())
		{
			throw "invalid instantiation id";
		}

		entity = entityInstantiation->second;
		entityInstantiationQueue.erase(entityInstantiation);
	}
	else
	{
		entity = instantiate(entityType);
	}

	entity->_interface->id = entityID;

	return entity;
}
