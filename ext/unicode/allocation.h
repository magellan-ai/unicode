#ifndef _UNICODE_ALLOCATION_H
#define _UNICODE_ALLOCATION_H

#include "ruby.h"

void* buffer_malloc(size_t size);
void* buffer_realloc(void* pointer, size_t size);
void buffer_free(void* pointer);

#ifdef UNICODE_FAULT_INJECTION
void Init_unicode_fault_injection(VALUE module);
#endif

#endif
