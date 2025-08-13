// LLM used
/*
 * dynstring.h - Simple dynamic string library (header-only)
 *
 * Usage:
 *   #define DYNSTRING_IMPLEMENTATION
 *   #include "dynstring.h"
 *
 * Example:
 *   string_t s;
 *   string_init(&s, "Hello");
 *   string_append(&s, ", world!");
 *   printf("%s\n", s.data);
 *   string_free(&s);
 *
 * License: MIT
 */

#ifndef DYNSTRING_H
#define DYNSTRING_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
/* Dynamic string type */
typedef struct {
    char *data;     /* Null-terminated string buffer */
    size_t length;  /* Current length (without null terminator) */
    size_t capacity;/* Allocated capacity (including null terminator) */
} string_t;

#define STRING_OK 0
#define STRING_ERR_OOM 1

#ifdef __cplusplus
extern "C" {
#endif

/* Function declarations */
int string_init(string_t *s, const char *init_str);
void string_free(string_t *s);
int string_reserve(string_t *s, size_t new_capacity);
int string_append(string_t *s, const char *str);
int string_append_n(string_t *s, const char *str, size_t n);
int string_insert(string_t *s, size_t index, const char *str);
void string_clear(string_t *s);
int string_append_char(string_t *s, char c); // <-- Added missing semicolon

#ifdef __cplusplus
}
#endif

/* ================= Implementation ================= */
#ifdef DYNSTRING_IMPLEMENTATION

static size_t string_next_capacity(size_t current, size_t needed) {
    size_t cap = current ? current : 16;
    while (cap < needed) {
        if (cap > (SIZE_MAX / 2)) { cap = needed; break; }
        cap *= 2;
    }
    return cap;
}

int string_init(string_t *s, const char *init_str) {
    if (!s) return STRING_ERR_OOM;
    s->length = init_str ? strlen(init_str) : 0;
    s->capacity = string_next_capacity(0, s->length + 1);
    s->data = (char*)malloc(s->capacity);
    if (!s->data) return STRING_ERR_OOM;
    if (init_str) memcpy(s->data, init_str, s->length);
    s->data[s->length] = '\0';
    return STRING_OK;
}

void string_free(string_t *s) {
    if (!s) return;
    free(s->data);
    s->data = NULL;
    s->length = s->capacity = 0;
}

int string_reserve(string_t *s, size_t new_capacity) {
    if (new_capacity <= s->capacity) return STRING_OK;
    char *p = (char*)realloc(s->data, new_capacity);
    if (!p) return STRING_ERR_OOM;
    s->data = p;
    s->capacity = new_capacity;
    return STRING_OK;
}

int string_append(string_t *s, const char *str) {
    if (!str) return STRING_OK;
    return string_append_n(s, str, strlen(str));
}

int string_append_n(string_t *s, const char *str, size_t n) {
    if (!s || !str) return STRING_ERR_OOM;
    size_t new_len = s->length + n;
    if (new_len + 1 > s->capacity) {
        size_t new_cap = string_next_capacity(s->capacity, new_len + 1);
        if (string_reserve(s, new_cap) != STRING_OK) return STRING_ERR_OOM;
    }
    memcpy(s->data + s->length, str, n);
    s->length = new_len;
    s->data[s->length] = '\0';
    return STRING_OK;
}

int string_insert(string_t *s, size_t index, const char *str) {
    if (!s || !str || index > s->length) return STRING_ERR_OOM;
    size_t len_str = strlen(str);
    size_t new_len = s->length + len_str;
    if (new_len + 1 > s->capacity) {
        size_t new_cap = string_next_capacity(s->capacity, new_len + 1);
        if (string_reserve(s, new_cap) != STRING_OK) return STRING_ERR_OOM;
    }
    memmove(s->data + index + len_str, s->data + index, s->length - index + 1);
    memcpy(s->data + index, str, len_str);
    s->length = new_len;
    return STRING_OK;
}

void string_clear(string_t *s) {
    if (!s) return;
    s->length = 0;
    if (s->data) s->data[0] = '\0';
}
int string_append_char(string_t *s, char c) {
    if (!s) return STRING_ERR_OOM;
    if (s->length + 2 > s->capacity) { // +2 for char and '\0'
        size_t new_cap = string_next_capacity(s->capacity, s->length + 2);
        if (string_reserve(s, new_cap) != STRING_OK) return STRING_ERR_OOM;
    }
    s->data[s->length] = c;
    s->length += 1;
    s->data[s->length] = '\0';
    return STRING_OK;
}

#endif /* DYNSTRING_IMPLEMENTATION */

#endif /* DYNSTRING_H */
// LLM usage ends
