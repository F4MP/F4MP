#include "f4se/PluginAPI.h"		// super
#include "f4se_common/f4se_version.h"	// What version of SKSE is running?
#include <shlobj.h>				// CSIDL_MYCODUMENTS

#include "f4mp.h"

static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
static F4SEPapyrusInterface* g_papyrus = NULL;

extern "C" {

	bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info) {	// Called by SKSE to learn about this plugin and check that it's safe to load it
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\F4MP.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_Error);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "F4MP";
		info->version = 1;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = f4se->GetPluginHandle();

		if (f4se->isEditor)
		{
			_MESSAGE("loaded in editor, marking as incompatible");

			return false;
		}
		else if (f4se->runtimeVersion != RUNTIME_VERSION_1_10_163)
		{
			_MESSAGE("unsupported runtime version %08X", f4se->runtimeVersion);

			return false;
		}

		// ### do not do anything else in this callback
		// ### only fill out PluginInfo and return true/false

		// supported runtime version
		return true;
	}

	bool F4SEPlugin_Load(const F4SEInterface* f4se) {	// Called by SKSE to load this plugin
		_MESSAGE("F4MP loaded");

		g_papyrus = (F4SEPapyrusInterface*)f4se->QueryInterface(kInterface_Papyrus);

		//Check if the function registration was a success...
		bool btest = g_papyrus->Register(F4MP::RegisterFuncs);

		if (btest) {
			_MESSAGE("Register succeeded");
		}

		return true;
	}

};