/* Minimal runtime for FreeRTOS demo: memset (required by tasks.c, heap_4.c). */
#include <stddef.h>

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}
