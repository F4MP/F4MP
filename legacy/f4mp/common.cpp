#include "common.h"

u64 f4mp::GetUniqueFormID(u32 ownerEntityID, u32 entityFormID)
{
	return ((u64)ownerEntityID << 32) | entityFormID;
}
