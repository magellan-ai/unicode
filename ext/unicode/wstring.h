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
  int len;
  int size;
} WString;

WString* WStr_alloc(WString* str);
WString* WStr_allocWithUTF8L(WString* s, const char* u, int len);
WString* WStr_enlarge(WString* str, int size);
void WStr_free(WString* str);
int WStr_addWChars(WString* s, const int* a, int len);
int WStr_addWChar(WString* s, int a);
int WStr_pushWString(WString* s, const WString* add);
UString* WStr_convertIntoUString(WString* wstr, UString* ustr);
UString* WStr_convertIntoUString2(WString* wstr, int start, int len, UString* ustr);

#ifdef __cplusplus
}
#endif

#endif
