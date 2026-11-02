/*
 * Simple string library
 * By Yoshida Masato, 1999. See LICENSE.
 */

#ifndef _USTRING_H
#define _USTRING_H

#ifdef __cplusplus
extern "C" {
#endif

#define USTR_INITIAL_STRING_LEN 1024
#define USTR_STRING_EXTEND_LEN 1024

typedef struct _UString {
  unsigned char* str;
  long len;
  long size;
} UString;

UString* UniStr_alloc(UString* str);
UString* UniStr_enlarge(UString* str, long size);
void UniStr_free(UString* str);
long UniStr_addChar(UString* s, unsigned char a);
long UniStr_addChar2(UString* s, unsigned char a1, unsigned char a2);
long UniStr_addChar3(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3);
long UniStr_addChar4(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4);
long UniStr_addChar5(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4, unsigned char a5);
long UniStr_addChar6(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4,
		  unsigned char a5, unsigned char a6);
long UniStr_addWChar(UString* s, unsigned int c);

#ifdef __cplusplus
}
#endif

#endif
