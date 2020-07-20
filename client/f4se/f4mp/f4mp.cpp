#include "F4MP.h"
#include "Librg.h"

#include "f4se/PluginManager.h"
#include "f4se/GameInput.h"
#include "f4se/GameMenus.h"

#include <memory>

namespace f4mp
{
	class InputHandler : public PlayerInputHandler
	{
		friend F4MP;

	public:
		InputHandler(F4MP& instance) : PlayerInputHandler(), instance(instance) { }

		~InputHandler()
		{
			PlayerControls* controls = *g_playerControls;
			if (controls == nullptr)
			{
				return;
			}

			PlayerInputHandler* _this = this;
			SInt64 index = controls->inputEvents1.GetItemIndex(_this);
			if (index < 0)
			{
				return;
			}

			controls->inputEvents1.Remove(index);
		}

		void OnButtonEvent(ButtonEvent* inputEvent)
		{
			if ((*g_ui)->numPauseGame)
			{
				return;
			}

			if (inputEvent->deviceType != InputEvent::kDeviceType_Keyboard)
			{
				return;
			}

			if (inputEvent->isDown != 1.f || inputEvent->timer != 0.f)
			{
				return;
			}

			switch (inputEvent->keyMask)
			{
			case 112:
				printf("F1\n");
				break;
			}
		}

	private:
		F4MP& instance;
	};
}

std::unique_ptr<f4mp::InputHandler> inputHandler;

f4mp::F4MP::F4MP(const F4SEInterface* f4se) : networking(nullptr), pluginHandle(kPluginHandle_Invalid)
{
	networking = new librg::Librg(false);

	inputHandler = std::make_unique<InputHandler>(*this);

	pluginHandle = f4se->GetPluginHandle();

	((F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging))->RegisterListener(pluginHandle, "F4SE", [](F4SEMessagingInterface::Message* msg)
		{
			switch (msg->type)
			{
			case F4SEMessagingInterface::kMessage_GameLoaded:
				(*g_playerControls)->inputEvents1.Push(inputHandler.get());
				break;
			}
		});
}

f4mp::F4MP::~F4MP()
{
	delete networking;
	networking = nullptr;
}