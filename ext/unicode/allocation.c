/*
 * Heap allocation for the extension's buffers. Built with UNICODE_FAULT_INJECTION (the sanitizer
 * build only), Unicode::FaultInjection can make the Nth allocation from now raise NoMemoryError,
 * as an exhausted ruby_xmalloc would, and counts the buffers still allocated, so specs can drive
 * every failure path and check that nothing leaks.
 */

#include "allocation.h"

#ifdef UNICODE_FAULT_INJECTION
/* NOT Ractor-safe, BUT this is only used in sanitizer builds, which can't run Ractors anyway. */
static long allocations_until_failure;
static long allocation_count;
static long live_buffers;

static void
count_allocation(void)
{
  allocation_count++;
  if (allocations_until_failure > 0 && --allocations_until_failure == 0)
    rb_memerror();
}
#  define track_buffer(change) (live_buffers += (change))
#else
#  define count_allocation() ((void)0)
#  define track_buffer(change) ((void)0)
#endif

void*
buffer_malloc(size_t count, size_t size)
{
  void* buffer;

  count_allocation();
  buffer = ruby_xmalloc2(count, size);
  track_buffer(1);
  return buffer;
}

void*
buffer_realloc(void* pointer, size_t count, size_t size)
{
  void* buffer;

  count_allocation();
  buffer = ruby_xrealloc2(pointer, count, size);
  if (!pointer)
    track_buffer(1);
  return buffer;
}

void
buffer_free(void* pointer)
{
  if (pointer)
    track_buffer(-1);
  ruby_xfree(pointer);
}

#ifdef UNICODE_FAULT_INJECTION
static VALUE
fail_allocation(VALUE module, VALUE nth)
{
  allocations_until_failure = NUM2LONG(nth);
  return Qnil;
}

static VALUE
allocations(VALUE module)
{
  return LONG2NUM(allocation_count);
}

static VALUE
live_buffer_count(VALUE module)
{
  return LONG2NUM(live_buffers);
}

void
Init_unicode_fault_injection(VALUE module)
{
  VALUE fault_injection = rb_define_module_under(module, "FaultInjection");

  rb_define_module_function(fault_injection, "fail_allocation", fail_allocation, 1);
  rb_define_module_function(fault_injection, "allocations", allocations, 0);
  rb_define_module_function(fault_injection, "live_buffers", live_buffer_count, 0);
}
#endif
