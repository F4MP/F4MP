#include "F4MP.h"
#include "Librg.h"

#include "f4se/GameInput.h"
#include "f4se/GameMenus.h"
#include "f4se/PluginManager.h"
	
class F4MPInputHandler : public PlayerInputHandler
{
public:
	F4MPInputHandler() : PlayerInputHandler() { }

	virtual void OnButtonEvent(ButtonEvent* inputEvent)
	{
		if ((*g_ui)->numPauseGame)
		{
			return;
		}

		switch (inputEvent->keyMask)
		{
		case 112:
			break;
		}
	}
};

F4MPInputHandler inputHandler;

f4mp::F4MP::F4MP(const F4SEInterface* f4se) : networking(nullptr), pluginHandle(kPluginHandle_Invalid)
{
	networking = new librg::Librg(false);

	pluginHandle = f4se->GetPluginHandle();

	((F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging))->RegisterListener(pluginHandle, "F4SE", [](F4SEMessagingInterface::Message* msg)
		{
			switch (msg->type)
			{
			case F4SEMessagingInterface::kMessage_GameLoaded:
				(*g_playerControls)->inputEvents1.Push(&inputHandler);
				break;
			}
		});
}

f4mp::F4MP::~F4MP()
{
	delete networking;
	networking = nullptr;
}