#define ZPL_IMPLEMENTATION
#define ZPL_NANO
#define ZPL_ENABLE_TIMER
#include <zpl.h>

zpl_global u8 counter = 0;

void test1_cb(void *user_data) {
    (void)user_data;
    zpl_printf("Hello, Sailor!\n");
    
    if (counter == 10) {
        zpl_printf("The second timer is done, press ^C to terminate the process.\n");
    }
}

void test2_cb(void *user_data) {
    (void)user_data;
    zpl_printf("This has been called %d/10 times!\n", ++counter);
}

int main(void) {
    
    zpl_timer_pool timers;
    zpl_array_init(timers, zpl_heap_allocator());
    
    zpl_timer *t1 = zpl_timer_add(timers);
    zpl_timer_set(t1, 4, -1, test1_cb);
    zpl_timer_start(t1, 0);
    
    zpl_timer *t2 = zpl_timer_add(timers);
    zpl_timer_set(t2, 1, 10, test2_cb);
    zpl_timer_start(t2, 2);
    
    zpl_timer *t3 = zpl_timer_add(timers);
    zpl_timer_set(t3, 1, 10, test2_cb);
    zpl_timer_start(t3, 2);
    zpl_timer_stop(t3); // NOTE(ZaKlaus): This won't be fired!
    
    while (1) {
        zpl_timer_update(timers);
    }
    
    zpl_array_free(timers);
    
    return 0;
}
