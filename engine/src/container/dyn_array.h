#pragma once

#include "define.h"

/* PERF:
 * Memory Layout
 * +-------------------+------------------+--------------------+------------------+
 * | DYNARRAY_CAPACITY | DYN_ARRAY_LENGTH | DYN_ARRAY_STRIDE   |   void* element  |
 * +-------------------+------------------+--------------------+------------------+
 * |       u64         |       u64        | u64 - size in byte | Actual elements  |
 * +-------------------+------------------+--------------------+------------------+
 */

enum
{
    DYN_ARRAY_CAPACITY,
    DYN_ARRAY_LENGTH,
    DYN_ARRAY_STRIDE,
    DYN_ARRAY_FIELD_LENGTH
};

ACAPI void* _array_create(u64 length, u64 stride);
ACAPI void _array_destroy(void* array);

ACAPI u64 _array_get_field(void* array, u64 field);
ACAPI void _array_set_field(void* array, u64 field, u64 value);

ACAPI void* _array_resize(void* array);

ACAPI void* _array_push(void* array, const void* value_ptr);
ACAPI void _array_pop(void* array, void* dest);

ACAPI void* _array_pop_at(void* array, u64 index, void* dest);
ACAPI void* _array_insert_at(void* array, u64 index, void* value_ptr);

#define DYN_ARRAY_DEF_CAPACITY 1
#define DYN_ARRAY_RESIZE_FACTOR 2

#define ac_dyn_array_create_t(type) _array_create(DYN_ARRAY_DEF_CAPACITY, sizeof(type))

// only set capacity of dynamic array but not length
#define ac_dyn_array_reserved_t(type, capacity) _array_create(capacity, sizeof(type))

#define ac_dyn_array_destroy_t(array) _array_destroy(array)

// ISSUE: still have a bug in this, on both platform even using GCC
#define ac_dyn_array_push_t(array, value)                                                                                                  \
    {                                                                                                                                      \
        __typeof__(value) temp = value;                                                                                                    \
        array = _array_push(array, &temp);                                                                                                 \
    }

#define ac_dyn_array_pop_t(array, value_ptr) _array_pop(array, value_ptr)

// ISSUE: still have a bug in this, on both platform even using GCC
#define ac_dyn_array_insert_at_t(array, index, value)                                                                                      \
    {                                                                                                                                      \
        __type__(value) temp = value;                                                                                                      \
        array = _array_insert_at(array, index, &temp);                                                                                     \
    }

#define ac_dyn_array_pop_at_t(array, index, value_ptr) _array_pop_at(array, index, value_ptr)

#define ac_dyn_array_clear_t(array) _array_set_field(array, DYN_ARRAY_LENGTH, 0)

#define ac_dyn_array_capacity_t(array) _array_get_field(array, DYN_ARRAY_CAPACITY)

#define ac_dyn_array_length_t(array) _array_get_field(array, DYN_ARRAY_LENGTH)

#define ac_dyn_array_stride_t(array) _array_get_field(array, DYN_ARRAY_STRIDE)

// this was for set length of dynamic array
#define ac_dyn_array_length_set_t(array, value) _array_set_field(array, DYN_ARRAY_LENGTH, value)
