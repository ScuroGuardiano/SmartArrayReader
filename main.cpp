#include "BUSE/buse.h"
#include <string>
#include <memory.h>

int read(void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
    for (int i = 0; i < len; i++)
    {
        memcpy(buf + i, "A", 1);
    }

    return 0;
}

int main(int argc, char** argv)
{
    buse_operations ops = {
        .read = read,
        .size = 8192
    };

    buse_main("/dev/nbd1", &ops, NULL);
}
