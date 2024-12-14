#pragma once

#include "define.h"

// Returns the length of the given string.
ACAPI u64 string_length(const char* str);
ACAPI char* string_duplicate(const char* str);

// Case-sensitive string compare. true if same, otherwise false
ACAPI b8 string_equal(const char* str0, const char* str1);
