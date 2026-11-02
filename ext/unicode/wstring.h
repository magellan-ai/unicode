/*
 * Simple wide string library
 * By Yoshida Masato, 1999. See LICENSE.
 */

#ifndef _WSTRING_H
#define _WSTRING_H

#include "ustring.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WSTR_INITIAL_STRING_LEN 1024
#define WSTR_STRING_EXTEND_LEN 1024

typedef struct _WString {
  int* str;
  long len;
  long size;
} WString;

WString* WStr_alloc(WString* str);
WString* WStr_allocCapacity(WString* str, long capacity);
WString* WStr_decodeUTF8(WString* s, const char* u, long len);
WString* WStr_enlarge(WString* str, long size);
void WStr_free(WString* str);
long WStr_addWChars(WString* s, const int* a, long len);
long WStr_addWChar(WString* s, int a);
long WStr_pushWString(WString* s, const WString* add);
UString* WStr_convertIntoUString(WString* wstr, UString* ustr);
UString* WStr_convertIntoUString2(WString* wstr, long start, long len, UString* ustr);

#ifdef __cplusplus
}
#endif

#endif
