// LLM generated
// vectorlib.h
#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*vector_destructor_fn)(void *);

typedef struct {
    void *data;
    size_t elem_size;
    size_t size;
    size_t capacity;
    vector_destructor_fn destructor;
} vector_t;

/* Error codes */
#define VECTOR_OK 0
#define VECTOR_ERR_OOM 1
#define VECTOR_ERR_BOUNDS 2

/* --- Public API declarations --- */
void vector_init(vector_t *vec, size_t elem_size, size_t capacity);
void vector_init_with_destructor(vector_t *vec, size_t elem_size, size_t capacity, vector_destructor_fn destructor);
void vector_free(vector_t *vec);
void vector_sort(vector_t *vec, int (*comparator)(const void *, const void *));
int vector_reserve(vector_t *v, size_t new_capacity);
int vector_shrink_to_fit(vector_t *v);
static inline size_t vector_size(const vector_t *v) { return v->size; }
static inline size_t vector_capacity(const vector_t *v) { return v->capacity; }
static inline void vector_clear(vector_t *v) { v->size = 0; }
int vector_push_back(vector_t *v, const void *elem_ptr);
int vector_pop_back(vector_t *v, void *out_elem);
void *vector_get(vector_t *v, size_t index);
int vector_set(vector_t *v, size_t index, const void *elem_ptr);
int vector_insert(vector_t *v, size_t index, const void *elem_ptr);
int vector_erase(vector_t *v, size_t index, void *out_elem);

#ifdef __cplusplus
}
#endif

#endif /* VECTOR_H */

/* ================= Implementation Section ================= */
#ifdef VECTOR_IMPLEMENTATION
#ifndef VECTOR_C_INCLUDED
#define VECTOR_C_INCLUDED

static inline size_t vector_next_capacity(size_t current, size_t min_needed) {
    size_t cap = current ? current : 4;
    while (cap < min_needed) {
        if (cap > (SIZE_MAX / 2)) { cap = min_needed; break; }
        cap *= 2;
    }
    return cap;
}

void vector_init(vector_t *vec, size_t elem_size, size_t capacity) {
    vec->elem_size = elem_size;
    vec->size = 0;
    vec->capacity = capacity > 0 ? capacity : 4;
    vec->data = malloc(vec->capacity * elem_size);
    vec->destructor = NULL;
}

void vector_init_with_destructor(vector_t *vec,
                                               size_t elem_size,
                                               size_t capacity,
                                               vector_destructor_fn destructor) {
    vector_init(vec, elem_size, capacity);
    vec->destructor = destructor;
}

void vector_free(vector_t *vec) {
    if (!vec) return;
    if (vec->destructor && vec->data) {
        for (size_t i = 0; i < vec->size; i++) {
            void *element = (char *)vec->data + i * vec->elem_size;
            vec->destructor(element);
        }
    }
    free(vec->data);
    vec->data = NULL;
    vec->size = vec->capacity = vec->elem_size = 0;
    vec->destructor = NULL;
}

void vector_sort(vector_t *vec, int (*comparator)(const void *, const void *)) {
    qsort(vec->data, vec->size, vec->elem_size, comparator);
}

int vector_reserve(vector_t *v, size_t new_capacity) {
    if (new_capacity <= v->capacity) return VECTOR_OK;
    void *p = realloc(v->data, new_capacity * v->elem_size);
    if (!p) return VECTOR_ERR_OOM;
    v->data = p;
    v->capacity = new_capacity;
    return VECTOR_OK;
}

int vector_push_back(vector_t *v, const void *elem_ptr) {
    if (v->size >= v->capacity) {
        size_t new_cap = vector_next_capacity(v->capacity, v->size + 1);
        if (vector_reserve(v, new_cap) != VECTOR_OK) {
            return VECTOR_ERR_OOM;
        }
    }
    memcpy((char*)v->data + (v->size * v->elem_size), elem_ptr, v->elem_size);
    v->size++;
    return VECTOR_OK;
}

void *vector_get(vector_t *v, size_t index) {
    if (index >= v->size) return NULL;
    return (char*)v->data + (index * v->elem_size);
}

int vector_pop_back(vector_t *v, void *out_elem) {
    if (v->size == 0) return VECTOR_ERR_BOUNDS;
    v->size--;
    if (out_elem != NULL) {
        memcpy(out_elem, (char*)v->data + (v->size * v->elem_size), v->elem_size);
    }
    return VECTOR_OK;
}

int vector_set(vector_t *v, size_t index, const void *elem_ptr) {
    if (index >= v->size) return VECTOR_ERR_BOUNDS;
    // Note: No destructor is called on the old element.
    memcpy((char*)v->data + (index * v->elem_size), elem_ptr, v->elem_size);
    return VECTOR_OK;
}

int vector_insert(vector_t *v, size_t index, const void *elem_ptr) {
    if (index > v->size) return VECTOR_ERR_BOUNDS;
    if (v->size >= v->capacity) {
        size_t new_cap = vector_next_capacity(v->capacity, v->size + 1);
        if (vector_reserve(v, new_cap) != VECTOR_OK) return VECTOR_ERR_OOM;
    }
    char *p = (char*)v->data + (index * v->elem_size);
    memmove(p + v->elem_size, p, (v->size - index) * v->elem_size);
    memcpy(p, elem_ptr, v->elem_size);
    v->size++;
    return VECTOR_OK;
}

int vector_erase(vector_t *v, size_t index, void *out_elem) {
    if (index >= v->size) return VECTOR_ERR_BOUNDS;
    char *p = (char*)v->data + (index * v->elem_size);
    if (out_elem != NULL) {
        memcpy(out_elem, p, v->elem_size);
    }
    memmove(p, p + v->elem_size, (v->size - index - 1) * v->elem_size);
    v->size--;
    return VECTOR_OK;
}

int vector_shrink_to_fit(vector_t *v) {
    if (v->size == v->capacity) return VECTOR_OK;
    void *p = realloc(v->data, v->size * v->elem_size);
    if (!p && v->size > 0) return VECTOR_ERR_OOM;
    v->data = p;
    v->capacity = v->size;
    return VECTOR_OK;
}

#endif /* VECTOR_C_INCLUDED */
#endif /* VECTOR_IMPLEMENTATION */

//LLM generated