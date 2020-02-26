// file: header/dll.h

/** @file dll.c
@brief DLL Handling
@defgroup dll DLL handling

@{
*/
#ifdef ZPL_EDITOR
#include <zpl.h>
#endif

ZPL_BEGIN_C_DECLS

typedef void *zpl_dll_handle;
typedef void (*zpl_dll_proc)(void);

ZPL_DEF zpl_dll_handle zpl_dll_load(char const *filepath);
ZPL_DEF void           zpl_dll_unload(zpl_dll_handle dll);
ZPL_DEF zpl_dll_proc   zpl_dll_proc_address(zpl_dll_handle dll, char const *proc_name);

//! @}

ZPL_END_C_DECLS
