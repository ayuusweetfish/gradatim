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
        size_t new_size = c * 2;
        if (new_size < s + bytes) new_size = s + bytes;
        ptr = (char *)realloc(ptr, 2 * sizeof(size_t) + new_size);
        if (ptr == NULL) {
            free(*b - 2 * sizeof(size_t));
            *b = NULL;
            return;
        }
        ptr += 2 * sizeof(size_t);
        bekter_capacity(ptr) = new_size;
        *b = ptr;
    }
}
