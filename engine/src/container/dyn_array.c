#include "container/dyn_array.h"
#include "core/acmemory.h"
#include "core/logger.h"

void* _array_create(u64 length, u64 stride)
{
    u64 size_header = DYN_ARRAY_FIELD_LENGTH * sizeof(u64);
    u64 size_array = length * stride;

    u64* array = ac_allocate_t(size_header + size_array, MEMTAG_DYN_ARRAY);
    ac_set_memory_t(array, 0, size_header + size_array);

    // this way was a way to only return the array itself.
    array[DYN_ARRAY_CAPACITY] = length;
    array[DYN_ARRAY_LENGTH] = 0;
    array[DYN_ARRAY_STRIDE] = stride;


    return (void*)(array + DYN_ARRAY_FIELD_LENGTH);
}

void _array_destroy(void* array)
{
    u64* header = (u64*)array - DYN_ARRAY_FIELD_LENGTH;
    u64 size_header = DYN_ARRAY_FIELD_LENGTH * sizeof(u64);
    u64 size_total = size_header + header[DYN_ARRAY_CAPACITY] * header[DYN_ARRAY_STRIDE];
    ac_free_t(header, size_total, MEMTAG_DYN_ARRAY);
}

u64 _array_get_field(void* array, u64 field)
{
    u64* header = (u64*)array - DYN_ARRAY_FIELD_LENGTH;
    return header[field];
}

void _array_set_field(void* array, u64 field, u64 value)
{
    u64* header = (u64*)array - DYN_ARRAY_FIELD_LENGTH;
    header[field] = value;
}

void* _array_resize(void* array)
{
    u64 length = ac_dyn_array_length_t(array);
    u64 stride = ac_dyn_array_stride_t(array);

    void* temp = _array_create(DYN_ARRAY_RESIZE_FACTOR * ac_dyn_array_capacity_t(array), stride);
    ac_copy_memory_t(temp, array, length * stride);

    _array_set_field(temp, DYN_ARRAY_LENGTH, length);
    _array_destroy(array);
    return temp;
}

void* _array_push(void* array, const void* value_ptr)
{
    u64 length = ac_dyn_array_length_t(array);
    u64 stride = ac_dyn_array_stride_t(array);
    if (length >= ac_dyn_array_capacity_t(array))
        array = _array_resize(array);

    u64 address = (u64)array;
    address += (length * stride);
    ac_copy_memory_t((void*)address, value_ptr, stride);
    _array_set_field(array, DYN_ARRAY_LENGTH, length + 1);
    return array;
}

void _array_pop(void* array, void* dest)
{
    u64 length = ac_dyn_array_length_t(array);
    u64 stride = ac_dyn_array_stride_t(array);
    u64 address = (u64)array;

    address += ((length - 1) * stride);
    ac_copy_memory_t(dest, (void*)address, stride);
    _array_set_field(array, DYN_ARRAY_LENGTH, length - 1);
}

void* _array_pop_at(void* array, u64 index, void* dest)
{
    u64 length = ac_dyn_array_length_t(array);
    u64 stride = ac_dyn_array_stride_t(array);
    if (index >= length)
    {
        ACERROR("Index outside of bounds! Length: %i, index: %index", length, index);
        return array;
    }

    u64 address = (u64)array;
    ac_copy_memory_t(dest, (void*)(address + (index * stride)), stride);

    if (index != length - 1)
        ac_copy_memory_t((void*)(address + (index * stride)), (void*)(address + ((index + 1) * stride)), stride * (length - index));

    _array_set_field(array, DYN_ARRAY_LENGTH, length - 1);
    return array;
}

void* _array_insert_at(void* array, u64 index, void* value_ptr)
{
    u64 length = ac_dyn_array_length_t(array);
    u64 stride = ac_dyn_array_stride_t(array);
    if (index >= length)
    {
        ACERROR("Index outside of bounds! Length: %i, index: %index", length, index);
        return array;
    }
    if (length >= ac_dyn_array_capacity_t(array))
        array = _array_resize(array);

    u64 address = (u64)array;

    if (index != length - 1)
        ac_copy_memory_t((void*)(address + ((index - 1) * stride)), (void*)(address + (index * stride)), stride * (length - index));

    ac_copy_memory_t((void*)(address + (index * stride)), value_ptr, stride);

    _array_set_field(array, DYN_ARRAY_LENGTH, length + 1);
    return array;
}
