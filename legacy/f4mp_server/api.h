#pragma once

#include "Server.h"

#if _WIN32

#define EXPORT extern "C" __declspec(dllexport)

#else

#define EXPORT

#endif