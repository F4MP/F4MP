#define ZPL_IMPLEMENTATION
#define ZPL_NANO
#include <zpl.h>

ZPL_RING_DECLARE(u32);
ZPL_RING_DEFINE(u32);

int main()
{
    zpl_ring_u32 pad={0};
    zpl_ring_u32_init(&pad, zpl_heap(), 3);
    zpl_ring_u32_append(&pad, 1);
    zpl_ring_u32_append(&pad, 2);
    zpl_ring_u32_append(&pad, 3);
    
    while (!zpl_ring_u32_empty(&pad)) {
        zpl_printf("Result is %d\n", *zpl_ring_u32_get(&pad));
    }
    
    zpl_ring_u32_free(&pad);
    
    return 0;
}

