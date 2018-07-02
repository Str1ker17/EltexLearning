#pragma once

#ifdef _MSC_VER
#undef __cplusplus
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

// A simple array-like collection with ability to grow, containing pointers
typedef struct __vector {
	uintptr_t *array;
	size_t size;
	size_t capacity;
	uint32_t _growth_factor; // multiplied to 1024 for faster processing
} VECTOR;

bool vector_init(VECTOR *vect, size_t initial_capacity);
bool vector_push_back(VECTOR *vect, uintptr_t value);
bool vector_pop_back(VECTOR *vect, uintptr_t *out_value);
bool vector_shrink_to_fit(VECTOR *vect);
uintptr_t *vector_array_ptr(VECTOR *vect);
