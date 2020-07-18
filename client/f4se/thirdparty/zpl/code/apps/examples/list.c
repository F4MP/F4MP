#define ZPL_IMPLEMENTATION
#define ZPL_NANO
#include "zpl.h"

int main(void)
{
    zpl_list s, *head, *cursor;
    zpl_list_init(&s, "it is optional to call init: ");
    head = cursor = &s;

    // since we can construct an element implicitly this way
    // the second field gets overwritten once we add it to a list.
    zpl_list a = {"hello"};
    cursor = zpl_list_add(cursor, &a);

    zpl_list b = {"world"};
    cursor = zpl_list_add(cursor, &b);

    zpl_list c = {"!!! OK"};
    cursor = zpl_list_add(cursor, &c);

    for (zpl_list *l=head; l; l=l->next) {
        zpl_printf("%s ", cast(char *)l->ptr);
    }
    zpl_printf("\n");

    return 0;
}

