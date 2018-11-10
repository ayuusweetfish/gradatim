#include "bekter.h"

#include <stdlib.h>

static const size_t BEKTER_RESERVE_SZ = 16;

bekter bekter_create()
{
    char *ptr = (char *)malloc(2 * sizeof(size_t) + BEKTER_RESERVE_SZ);
    ptr += 2 * sizeof(size_t);
    bekter_size(ptr) = 0;
    bekter_capacity(ptr) = BEKTER_RESERVE_SZ;
    return ptr;
}

void bekter_drop(bekter b)
{
    free(b - 2 * sizeof(size_t));
}

void bekter_ensure_space(bekter *b, size_t bytes)
{
    size_t s = bekter_size(*b), c = bekter_capacity(*b);
    if (s + bytes > c) {
        char *ptr = *b - 2 * sizeof(size_t);
        ptr = (char *)realloc(ptr, 2 * sizeof(size_t) + c * 2);
        ptr += 2 * sizeof(size_t);
        bekter_capacity(ptr) = c * 2;
        *b = ptr;
    }
}
