#define LIBRG_IMPLEMENTATION
#define LIBRG_DEBUG
#define librg_log _DMESSAGE
#define ZPL_ASSERT_MSG _ERROR

#include "f4mp.h"

std::unique_ptr<F4MP> F4MP::instance;
