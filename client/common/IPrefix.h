#pragma once

// 4018 - signed/unsigned mismatch
// 4200 - zero-sized array
// 4244 - loss of data by assignment
// 4267 - possible loss of data (truncation)
// 4305 - truncation by assignment
// 4288 - disable warning for crap microsoft extension screwing up the scope of variables defined in for loops
// 4311 - pointer truncation
// 4312 - pointer extension
#pragma warning(disable: 4018 4200 4244 4267 4305 4288 4312 4311)

// winxp and above
//#define _WIN32_WINNT	0x0501

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include "common/ITypes.h"
#include "common/IErrors.h"
#include "common/IDynamicCreate.h"
#include "common/IDebugLog.h"
#include "common/ISingleton.h"
#include <winsock2.h>
#include <Windows.h>
