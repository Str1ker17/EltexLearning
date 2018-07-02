#include "vector.h"
#include <stdlib.h>

bool vector_init(VECTOR *vect, size_t initial_capacity) {
	if((vect->array = (uintptr_t*)malloc(sizeof(uintptr_t) * initial_capacity)) == NULL)
		return false;
	vect->_growth_factor = 1536;
	vect->size = 0;
	vect->capacity = initial_capacity;
	return true;
}

bool vector_resize(VECTOR *vect, size_t new_capacity) {
	//if(new_capacity <= vect->capacity)
	//	return true; // already have
	void *mem = realloc(vect->array, sizeof(uintptr_t) * new_capacity);
	if(mem == NULL)
		return false; // could not realloc
	vect->array = mem;
	return true;
}

bool vector_push_back(VECTOR *vect, uintptr_t value) {
	if(vect->size >= vect->capacity) {
		if(!vector_resize(vect, max(vect->size + 1, vect->capacity * vect->_growth_factor / 1024)))
			return false;
	}
	vect->array[vect->size] = value;
	++vect->size;
	return true;
}

bool vector_pop_back(VECTOR *vect, uintptr_t *out_value) {
	if(vect->size == 0)
		return false;
	--vect->size;
	*out_value = vect->array[vect->size];
	return true;
}

bool vector_shrink_to_fit(VECTOR *vect) {
	if(!vector_resize(vect, vect->size))
		return false;
	vect->capacity = vect->size;
	return true;
}

uintptr_t *vector_array_ptr(VECTOR *vect) {
	return vect->array;
}