#pragma once

#include "define.h"

typedef enum mem_tag
{
    MEMTAG_UNKNOWN = 0,
    MEMTAG_ARRAY,
    MEMTAG_DYN_ARRAY,
    MEMTAG_DICT,
    MEMTAG_RING_QUEUE,
    MEMTAG_BST,
    MEMTAG_STRING,
    MEMTAG_APPLICATION,
    MEMTAG_JOB,
    MEMTAG_TEXTURE,
    MEMTAG_MATERIAL_INST,
    MEMTAG_RENDERER,
    MEMTAG_GAME,
    MEMTAG_TRANSFORM,
    MEMTAG_ENTITY,
    MEMTAG_ENTITY_NODE,
    MEMTAG_SCENE,

    MEMTAG_MAX_TAGS,
} mem_tag;

ACAPI void memory_initialize();
ACAPI void memory_shutdown();

ACAPI void* ac_allocate_t(u64 size, mem_tag tag);
ACAPI void ac_free_t(void* block, u64 size, mem_tag tag);
ACAPI void* ac_zero_memory_t(void* block, u64 size);
ACAPI void* ac_copy_memory_t(void* dest, const void* source, u64 size);
ACAPI void* ac_set_memory_t(void* dest, i32 value, u64 size);
ACAPI char* ac_get_memory_usage_t();
