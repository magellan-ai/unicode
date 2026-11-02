/*
 * Simple wide string library
 * By Yoshida Masato, 1999. See LICENSE.
 */

#include <stdlib.h>
#include <string.h>
#include "wstring.h"
#include "allocation.h"

WString*
WStr_alloc(WString* str)
{
  return WStr_allocCapacity(str, WSTR_INITIAL_STRING_LEN);
}

WString*
WStr_allocCapacity(WString* str, long capacity)
{
  str->str = (int*)buffer_malloc((size_t)capacity, sizeof(int));
  str->size = capacity;
  str->len = 0;

  return str;
}

WString*
WStr_enlarge(WString* str, long size)
{
  str->str = (int*)buffer_realloc(str->str, (size_t)(str->size + size), sizeof(int));
  str->size += size;

  return str;
}

void
WStr_free(WString* str)
{
  str->size = 0;
  str->len = 0;
  if (str->str) {
    buffer_free(str->str);
    str->str = NULL;
  }
}

long
WStr_addWChars(WString* s, const int* a, long len)
{
  if (s->len + len >= s->size) {
    WStr_enlarge(s, len + WSTR_STRING_EXTEND_LEN);
  }
  memcpy(s->str + s->len, a, len * sizeof(int));
  s->len += len;

  return s->len;
}

long
WStr_addWChar(WString* s, int a)
{
  if (s->len + 1 >= s->size) {
    WStr_enlarge(s, WSTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a;
  (s->len)++;

  return s->len;
}

long
WStr_pushWString(WString* s, const WString* add)
{
  if (s->len + add->len >= s->size) {
    WStr_enlarge(s, add->len + WSTR_STRING_EXTEND_LEN);
  }
  memcpy(s->str + s->len, add->str, add->len * sizeof(int));
  s->len += add->len;

  return s->len;
}

/*
 * Appends the codepoints of len bytes of UTF-8 to s, which must have room for len more. Never allocates. Returns
 * NULL instead of decoding any part of a malformed sequence (a stray continuation byte, a character that starts
 * before the previous one is complete, or input that ends mid-character).
 */
WString*
WStr_decodeUTF8(WString* s, const char* in, long len)
{
  long i;
  int u = 0;
  int rest = 0;

  if (in == NULL)
    return s;
  for (i = 0; i < len; i++) {
    unsigned char c = in[i];
    if ((c & 0xc0) != 0x80 && rest != 0)
      return NULL;
    if ((c & 0xc0) == 0x80) {
      if (rest == 0)
	return NULL;
      u = (u << 6) | (c & 63);
      rest--;
      if (rest == 0) {
	WStr_addWChar(s, u);
      }
    }
    else if ((c & 0x80) == 0) {      /* 0b0nnnnnnn (7bit) */
      WStr_addWChar(s, c);
    }
    else if ((c & 0xe0) == 0xc0) {      /* 0b110nnnnn (11bit) */
      rest = 1;
      u = c & 31;
    }
    else if ((c & 0xf0) == 0xe0) {      /* 0b1110nnnn (16bit) */
      rest = 2;
      u = c & 15;
    }
    else if ((c & 0xf8) == 0xf0) {      /* 0b11110nnn (21bit) */
      rest = 3;
      u = c & 7;
    }
    else if ((c & 0xfc) == 0xf8) {      /* 0b111110nn (26bit) */
      rest = 4;
      u = c & 3;
    }
    else if ((c & 0xfe) == 0xfc) {      /* 0b1111110n (31bit) */
      rest = 5;
      u = c & 1;
    }
    else {
      return NULL;
    }
  }
  if (rest != 0)
    return NULL;

  return s;
}

UString*
WStr_convertIntoUString(WString* wstr, UString* ustr)
{
  long i;

  for (i = 0; i < wstr->len; i++) {
    UniStr_addWChar(ustr, wstr->str[i]);
  }

  return ustr;
}

UString*
WStr_convertIntoUString2(WString* wstr, long start, long len, UString* ustr)
{
  long i;

  for (i = start; i < wstr->len && i < start + len; i++) {
    UniStr_addWChar(ustr, wstr->str[i]);
  }

  return ustr;
}
