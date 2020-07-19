#include "F4MP.h"
#include "Librg.h"

f4mp::F4MP::F4MP() : networking(nullptr)
{
	networking = new librg::Librg();
}

f4mp::F4MP::~F4MP()
{
	delete networking;
	networking = nullptr;
}