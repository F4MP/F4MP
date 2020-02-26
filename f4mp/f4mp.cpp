#define LIBRG_IMPLEMENTATION
#define LIBRG_DEBUG
#define librg_log _DMESSAGE
//#define ZPL_ASSERT_MSG _ERROR

#define ZPL_ASSERT_MSG(cond, msg, ...)\
    do {\
        if (!(cond)) {\
            _ERROR(msg, ##__VA_ARGS__);\
        }\
    } while (0)

#include "f4mp.h"

std::vector<std::unique_ptr<f4mp::F4MP>> f4mp::F4MP::instances;
size_t f4mp::F4MP::activeInstance = 0;
