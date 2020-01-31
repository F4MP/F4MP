#define LIBRG_IMPLEMENTATION
#define LIBRG_DEBUG
#define librg_log _DMESSAGE
//#define ZPL_ASSERT_MSG _ERROR

#define ZPL_ASSERT_MSG(cond, msg, ...)                                                                             \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            _ERROR(msg, ##__VA_ARGS__);                           \
        }                                                                                                              \
    } while (0)

#include "f4mp.h"

std::unique_ptr<F4MP> F4MP::instance;
