#define ZPL_IMPLEMENTATION
#define ZPL_NANO
#include "zpl.h"

////////////////////////////////////////////////////////////
//
// Hash table type and function declaration, call: ZPL_TABLE_DECLARE(PREFIX, NAME, FUNC, VALUE)
// Hash table function definitions, call: ZPL_TABLE_DEFINE(NAME, FUNC, VALUE)
//
//     PREFIX  - a prefix for function prototypes e.g. extern, static, etc.
//     NAME    - Name of the Hash Table
//     FUNC    - the name will prefix function names
//     VALUE   - the type of the value to be stored
//


// NOTE(ZaKlaus): Declare user structure
typedef struct user {
    zpl_string name;
    i64          score;
} user;

// NOTE(ZaKlaus): Declare hash table of type user
ZPL_TABLE_DECLARE(extern, userbl, tbl_user_, user);

// NOTE(ZaKlaus): Define hash table functions
// Place exactly in __ONE__ source file!
ZPL_TABLE_DEFINE(userbl, tbl_user_, user);

int
main(void) {
    
    // NOTE(ZaKlaus): Create new hash table
    userbl users;
    tbl_user_init(&users, zpl_heap_allocator());
    
    // NOTE(ZaKlaus): Create hash key
    u64 key = 0x29;
    
    // NOTE(ZaKlaus): Create new user
    user user1;
    user1.name  = zpl_string_make(zpl_heap_allocator(), "John Doe");
    user1.score = 2674;
    
    // NOTE(ZaKlaus): Add user to the hash table
    tbl_user_set(&users, key, user1);
    
    // NOTE(ZaKlaus): Create another user
    user user2;
    user2.name  = zpl_string_make(zpl_heap_allocator(), "Steve L.");
    user2.score = 303312;
    
    // NOTE(ZaKlaus): Add user to the hash table
    tbl_user_set(&users, key+0xDEADBEEF, user2);
    
    // NOTE(ZaKlaus): Retrieve the first user
    user user3 = *tbl_user_get(&users, key);
    
    // NOTE(ZaKlaus): Make sure the user is identical to the first user
    ZPL_ASSERT(user3.score == user1.score);
    
    // NOTE(ZaKlaus): Release resources
    zpl_string_free(user1.name);
    zpl_string_free(user2.name);
    
    tbl_user_destroy(&users);
    
    return 0;
}
