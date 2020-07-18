#define ZPL_IMPLEMENTATION
#define ZPL_NANO
#include "zpl.h"

int
main(void) {
    
#define ARRAY_SIZE 4096*8
    // NOTE(ZaKlaus): Define a storage for our numbers and the temporary buffer.
    u32 nums[ARRAY_SIZE] = {0};
    u32 temp[ARRAY_SIZE] = {0};
    zpl_f64_cmp(0);
    
    // NOTE(ZaKlaus): Initialize random seed.
    // and generate random values.
    zpl_random r; zpl_random_init(&r);
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        nums[i] = zpl_random_gen_u32_unique(&r);
    }
    
    // NOTE(ZaKlaus): Print these values out
    zpl_printf("First 30 numbers...\nBefore: ");
    for (int i = 0; i < 90; ++i) {
        zpl_printf("%d, ", nums[i]);
    }
    
    // NOTE(ZaKlaus): Use radix sort on our numbers and print them out again.
    zpl_radix_sort_u32(nums, temp, ARRAY_SIZE);
    
    zpl_printf("\n\nAfter: ");
    for (int i = 0; i < 90; ++i) {
        zpl_printf("%d, ", nums[i]);
    }    
    
    return 0;
}
