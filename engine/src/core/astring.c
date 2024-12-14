#include "core/astring.h"
#include "core/acmemory.h"
#include <string.h>

u64 string_length(const char* str)
{
    return strlen(str);
}

char* string_duplicate(const char* str)
{
    u64 length = string_length(str);
    char* copy = ac_allocate_t(length + 1, MEMTAG_STRING);
    ac_copy_memory_t(copy, str, length + 1);
    return copy;
}

b8 string_equal(const char *str0, const char *str1)
{
    return strcmp(str0, str1);
}
