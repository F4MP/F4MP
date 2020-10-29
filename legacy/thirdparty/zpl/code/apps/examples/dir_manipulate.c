#define ZPL_IMPLEMENTATION
#define ZPL_NANO
#include <zpl.h>

int main() {
    zpl_path_mkdir("./tempdir/", 0733);
    zpl_path_mkdir("./tempdir/subdir", 0755);

    if (zpl_path_mkdir("./tempdir/", 0733) == ZPL_FILE_ERROR_EXISTS) {
        zpl_printf("error: directory already exist\n");
    }

    if (zpl_path_rmdir("./tempdir") == ZPL_FILE_ERROR_NOT_EMPTY) {
        zpl_printf("error: directory is not empty\n");
    }

    ZPL_ASSERT(zpl_path_rmdir("./tempdir/subdir") == ZPL_FILE_ERROR_NONE);
    ZPL_ASSERT(zpl_path_rmdir("./tempdir/") == ZPL_FILE_ERROR_NONE);

    return 0;
}
