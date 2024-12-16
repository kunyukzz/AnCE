#include "acmemory.h"

#include "core/astring.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <stdio.h>
#include <string.h>

struct mem_stats
{
    u64 total_allocated;
    u64 tagged_allocation[MEMTAG_MAX_TAGS];
};

static const char* memtag_string[MEMTAG_MAX_TAGS] = { "UNKNOWN", "ARRAY",       "DYN_ARRAY", "DICT",        "RING_QUEUE", "BST",
                                                      "STRING",  "APPLICATION", "JOB",    "TEXTURE",     "MAT_INST",   "RENDERER",
                                                      "GAME",    "TRANSFORM",   "ENTITY", "ENTITY_NODE", "SCENE" };

static struct mem_stats stats;

void memory_initialize()
{
    platform_zero_mem(&stats, sizeof(stats));
}

void memory_shutdown() {}

void* ac_allocate_t(u64 size, mem_tag tag)
{
    if (tag == MEMTAG_UNKNOWN)
        ACWARN("ac_allocated called using MEMTAG_UNKNOWN, Re-Class this allocation");

    stats.total_allocated += size;
    stats.tagged_allocation[tag] += size;

    // TODO : set memory allignment
    void* block = platform_allocated(size, FALSE);
    platform_zero_mem(block, size);

    return block;
}

void ac_free_t(void* block, u64 size, mem_tag tag)
{
    if (tag == MEMTAG_UNKNOWN)
        ACWARN("ac_free called using MEMTAG_UNKNOWN, Re-Class this allocation");

    stats.total_allocated -= size;
    stats.tagged_allocation[tag] -= size;

    // TODO : set memory allignment
    platform_free(block, FALSE);
}

void* ac_zero_memory_t(void* block, u64 size)
{
    return platform_zero_mem(block, size);
}

void* ac_copy_memory_t(void* dest, const void* source, u64 size)
{
    return platform_copy_mem(dest, source, size);
}

void* ac_set_memory_t(void* dest, i32 value, u64 size)
{
    return platform_set_mem(dest, value, size);
}

char* ac_get_memory_usage_t()
{
    const u64 Gib = 1024 * 1024 * 1024;
    const u64 Mib = 1024 * 1024;
    const u64 Kib = 1024;

    char buffer[8000] = "System memory used (tagged):\n";
    u64 offset = strlen(buffer);
    for (u32 i = 0; i < MEMTAG_MAX_TAGS; ++i)
    {
        char unit[4] = "XiB";
        float amount = 1.0f;
        if (stats.tagged_allocation[i] >= Gib)
        {
            unit[0] = 'G';
            amount = stats.tagged_allocation[i] / (float)Gib;
        }
        else if (stats.tagged_allocation[i] >= Mib)
        {
            unit[0] = 'M';
            amount = stats.tagged_allocation[i] / (float)Mib;
        }
        else if (stats.tagged_allocation[i] >= Kib)
        {
            unit[0] = 'K';
            amount = stats.tagged_allocation[i] / (float)Kib;
        }
        else
        {
            unit[0] = 'B';
            unit[1] = 0;
            amount = (float)stats.tagged_allocation[i];
        }

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memtag_string[i], amount, unit);
        offset += length;
    }

    //char* out_string;
    char* out_string = string_duplicate(buffer);
    return out_string;
}
