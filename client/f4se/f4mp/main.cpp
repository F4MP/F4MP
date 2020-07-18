#include "f4se/PluginAPI.h"		// super
#include "f4se_common/f4se_version.h"	// What version of SKSE is running?
#include <shlobj.h>				// CSIDL_MYCODUMENTS

#include "f4mp.h"

extern "C" {

	bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info) {	// Called by SKSE to learn about this plugin and check that it's safe to load it
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4MP\\F4MP.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_Error);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "F4MP";
		info->version = 1;

		// store plugin handle so we can identify ourselves later
		// g_pluginHandle = f4se->GetPluginHandle();

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

		//Check if the function registration was a success...
		bool btest = f4mp::F4MP::GetInstance().Init(f4se);

		if (btest) {
			_MESSAGE("Register succeeded");
		}

		return true;
	}

};