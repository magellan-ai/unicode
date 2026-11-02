/*
 * Unicode normalization, case mapping, and character properties for Ruby.
 * Original library by Yoshida Masato, 1999-2013. See LICENSE.
 */

#include "ruby.h"
#ifdef HAVE_RUBY_IO_H
#  include "ruby/io.h"
#else
#  include "rubyio.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include "wstring.h"
#include "unidata.map"

#ifndef RSTRING_PTR
#  define RSTRING_PTR(s) (RSTRING(s)->ptr)
#  define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#ifdef HAVE_RUBY_ENCODING_H
static rb_encoding* enc_out;
#  define ENC_(o) (rb_enc_associate(o, enc_out))
#else
#  define ENC_(o) (o)
#endif

inline static VALUE
taintObject(VALUE src, VALUE obj) {
  // deprecated: taintedness turned out to be a wrong idea
  //if (OBJ_TAINTED(src))
  //  OBJ_TAINT(obj);
  return obj;
}
#define TO_(src, obj) (taintObject(src, obj))

/*
 * Returns str as the valid UTF-8 String every entry point works on. UTF-8- and US-ASCII-tagged
 * strings are read as UTF-8 bytes; any other encoding is transcoded to UTF-8 first. Raises
 * TypeError for a non-String, an EncodingError when transcoding fails, and ArgumentError for bytes
 * that aren't valid UTF-8, which the decoder would otherwise silently truncate. Entry points call
 * this before allocating any native buffer, so a raise here leaks nothing.
 */
static VALUE
utf8_argument(VALUE str)
{
  int encindex;

  Check_Type(str, T_STRING);
  encindex = ENCODING_GET(str);
  if (encindex == rb_usascii_encindex()) {
    if (rb_enc_str_coderange(str) != ENC_CODERANGE_7BIT)
      str = rb_enc_associate(rb_str_dup(str), enc_out);
  }
  else if (encindex != rb_utf8_encindex()) {
    str = rb_str_encode(str, rb_enc_from_encoding(enc_out), 0, Qnil);
  }
  if (rb_enc_str_coderange(str) == ENC_CODERANGE_BROKEN)
    rb_raise(rb_eArgError, "invalid byte sequence in UTF-8");
  return str;
}

static VALUE mUnicode;
static VALUE catname_long[c_Cn+1];
static VALUE catname_abbr[c_Cn+1];

/* Hangul */
#define SBASE   (0xac00)
#define LBASE   (0x1100)
#define LCOUNT  (19)
#define VBASE   (0x1161)
#define VCOUNT  (21)
#define TBASE   (0x11a7)
#define TCOUNT  (28)
#define NCOUNT  (VCOUNT * TCOUNT) /* 588 */
#define SCOUNT  (LCOUNT * NCOUNT) /* 11172 */

/*
 * Character data lookups.
 *
 * unidata[] (unidata.map) is static, sorted by code in strictly ascending order, and ends with a
 * -1 sentinel. bmp_index and composition_pairs are built once by Init_unicode_native and never
 * change after that. No lookup calls the Ruby API, allocates, or raises, so lookups are safe while
 * native buffers are live, and none of this data is a Ruby object or a GC root.
 */

#define UNIDATA_COUNT (sizeof(unidata) / sizeof(unidata[0]) - 1)
#define BMP_SIZE 0x10000

_Static_assert(sizeof(unidata) / sizeof(unidata[0]) <= 0xFFFF, "bmp_index stores entry positions plus one in 16 bits");

/* For each codepoint below U+10000, its unidata[] position plus one, or 0 when it has no entry. */
static unsigned short bmp_index[BMP_SIZE];

struct composition_pair {
  int first;
  int second;
  int composite;
};

static struct composition_pair* composition_pairs;
static size_t composition_pair_count;

static const struct unicode_data*
find_unidata(int ucs)
{
  size_t low = 0;
  size_t high = UNIDATA_COUNT;

  if (ucs >= 0 && ucs < BMP_SIZE)
    return bmp_index[ucs] ? &unidata[bmp_index[ucs] - 1] : NULL;

  while (low < high) {
    size_t middle = low + (high - low) / 2;
    int code = unidata[middle].code;

    if (code == ucs)
      return &unidata[middle];
    if (code < ucs)
      low = middle + 1;
    else
      high = middle;
  }
  return NULL;
}

/*
 * Like find_unidata, but a character inside a range that UnicodeData.txt lists only by its first
 * and last code (CJK ideographs, Hangul syllables, surrogates, private use) gets the range's entry.
 */
static const struct unicode_data*
find_unidata_in_ranges(int ucs)
{
  const struct unicode_data* data = find_unidata(ucs);

  if (data)
    return data;
#ifdef CJK_IDEOGRAPH_EXTENSION_A_FIRST
  if (ucs >= CJK_IDEOGRAPH_EXTENSION_A_FIRST && ucs <= CJK_IDEOGRAPH_EXTENSION_A_LAST)
    return find_unidata(CJK_IDEOGRAPH_EXTENSION_A_FIRST);
#endif
#ifdef CJK_IDEOGRAPH_FIRST
  if (ucs >= CJK_IDEOGRAPH_FIRST && ucs <= CJK_IDEOGRAPH_LAST)
    return find_unidata(CJK_IDEOGRAPH_FIRST);
#endif
#ifdef HANGUL_SYLLABLE_FIRST
  if (ucs >= HANGUL_SYLLABLE_FIRST && ucs <= HANGUL_SYLLABLE_LAST)
    return find_unidata(HANGUL_SYLLABLE_FIRST);
#endif
#ifdef NON_PRIVATE_USE_HIGH_SURROGATE_FIRST
  if (ucs >= NON_PRIVATE_USE_HIGH_SURROGATE_FIRST && ucs <= NON_PRIVATE_USE_HIGH_SURROGATE_LAST)
    return find_unidata(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST);
#endif
#ifdef PRIVATE_USE_HIGH_SURROGATE_FIRST
  if (ucs >= PRIVATE_USE_HIGH_SURROGATE_FIRST && ucs <= PRIVATE_USE_HIGH_SURROGATE_LAST)
    return find_unidata(PRIVATE_USE_HIGH_SURROGATE_FIRST);
#endif
#ifdef LOW_SURROGATE_FIRST
  if (ucs >= LOW_SURROGATE_FIRST && ucs <= LOW_SURROGATE_LAST)
    return find_unidata(LOW_SURROGATE_FIRST);
#endif
#ifdef PRIVATE_USE_FIRST
  if (ucs >= PRIVATE_USE_FIRST && ucs <= PRIVATE_USE_LAST)
    return find_unidata(PRIVATE_USE_FIRST);
#endif
#ifdef CJK_IDEOGRAPH_EXTENSION_B_FIRST
  if (ucs >= CJK_IDEOGRAPH_EXTENSION_B_FIRST && ucs <= CJK_IDEOGRAPH_EXTENSION_B_LAST)
    return find_unidata(CJK_IDEOGRAPH_EXTENSION_B_FIRST);
#endif
#ifdef CJK_IDEOGRAPH_EXTENSION_C_FIRST
  if (ucs >= CJK_IDEOGRAPH_EXTENSION_C_FIRST && ucs <= CJK_IDEOGRAPH_EXTENSION_C_LAST)
    return find_unidata(CJK_IDEOGRAPH_EXTENSION_C_FIRST);
#endif
#ifdef CJK_IDEOGRAPH_EXTENSION_D_FIRST
  if (ucs >= CJK_IDEOGRAPH_EXTENSION_D_FIRST && ucs <= CJK_IDEOGRAPH_EXTENSION_D_LAST)
    return find_unidata(CJK_IDEOGRAPH_EXTENSION_D_FIRST);
#endif
#ifdef PLANE_15_PRIVATE_USE_FIRST
  if (ucs >= PLANE_15_PRIVATE_USE_FIRST && ucs <= PLANE_15_PRIVATE_USE_LAST)
    return find_unidata(PLANE_15_PRIVATE_USE_FIRST);
#endif
#ifdef PLANE_16_PRIVATE_USE_FIRST
  if (ucs >= PLANE_16_PRIVATE_USE_FIRST && ucs <= PLANE_16_PRIVATE_USE_LAST)
    return find_unidata(PLANE_16_PRIVATE_USE_FIRST);
#endif
  return NULL;
}

static int
get_cc(int ucs)
{
  const struct unicode_data* data = find_unidata(ucs);

  return data ? data->combining_class : 0;
}

static int
get_gencat(int ucs)
{
  const struct unicode_data* data = find_unidata_in_ranges(ucs);

  return data ? data->general_category : c_Cn; /* Unassigned */
}

static int
get_eawidth(int ucs)
{
  const struct unicode_data* data = find_unidata_in_ranges(ucs);

  return data ? data->east_asian_width : w_N; /* Neutral */
}

static const char*
get_canon(int ucs)
{
  const struct unicode_data* data = find_unidata(ucs);

  return data ? data->canon : NULL;
}

static const char*
get_canon_ex(int ucs)
{
  const struct unicode_data* data = find_unidata(ucs);

  return data && !data->exclusion ? data->canon : NULL;
}

static const char*
get_compat(int ucs)
{
  const struct unicode_data* data = find_unidata(ucs);

  return data ? data->compat : NULL;
}

static const char*
get_uppercase(int ucs)
{
  const struct unicode_data* data = find_unidata(ucs);

  return data ? data->uppercase : NULL;
}

static const char*
get_lowercase(int ucs)
{
  const struct unicode_data* data = find_unidata(ucs);

  return data ? data->lowercase : NULL;
}

static const char*
get_titlecase(int ucs)
{
  const struct unicode_data* data = find_unidata(ucs);

  return data ? data->titlecase : NULL;
}

static void
build_bmp_index(void)
{
  size_t i;

  for (i = 0; i < UNIDATA_COUNT && unidata[i].code < BMP_SIZE; i++)
    bmp_index[unidata[i].code] = (unsigned short)(i + 1);
}

static int
compare_composition_pairs(const void* left, const void* right)
{
  const struct composition_pair* a = left;
  const struct composition_pair* b = right;

  if (a->first != b->first)
    return a->first < b->first ? -1 : 1;
  if (a->second != b->second)
    return a->second < b->second ? -1 : 1;
  return 0;
}

static int
get_composition(int first, int second)
{
  struct composition_pair key = { first, second, 0 };
  const struct composition_pair* pair =
    bsearch(&key, composition_pairs, composition_pair_count, sizeof(key), compare_composition_pairs);

  return pair ? pair->composite : -1;
}

/*
 * Every canonical decomposition that may be recomposed (no composition exclusion) is exactly two
 * codepoints in Unicode 6.2; compose_pair only ever asks about two, so any other length is skipped.
 * The table is allocated once and kept for the life of the process. ALLOC_N raises NoMemoryError
 * before anything is assigned, so a failure here leaves nothing half-built.
 */
static void
build_composition_pairs(void)
{
  size_t i;
  size_t capacity = 0;

  for (i = 0; i < UNIDATA_COUNT; i++) {
    if (unidata[i].canon && !unidata[i].exclusion)
      capacity++;
  }

  composition_pairs = ALLOC_N(struct composition_pair, capacity);
  for (i = 0; i < UNIDATA_COUNT; i++) {
    WString decomposition;

    if (!unidata[i].canon || unidata[i].exclusion)
      continue;
    WStr_allocWithUTF8(&decomposition, unidata[i].canon);
    if (decomposition.len == 2) {
      composition_pairs[composition_pair_count].first = decomposition.str[0];
      composition_pairs[composition_pair_count].second = decomposition.str[1];
      composition_pairs[composition_pair_count].composite = unidata[i].code;
      composition_pair_count++;
    }
    WStr_free(&decomposition);
  }
  qsort(composition_pairs, composition_pair_count, sizeof(struct composition_pair), compare_composition_pairs);
}

static WString*
sort_canonical(WString* ustr)
{
  int i = 1;
  int len = ustr->len;

  if (len < 2) return ustr;

  while (i < len) {
    int last = ustr->str[i - 1];
    int ch = ustr->str[i];
    int last_cc = get_cc(last);
    int cc = get_cc(ch);
    if (cc != 0 && last_cc != 0 && last_cc > cc) {
      ustr->str[i] = last;
      ustr->str[i-1] = ch;
      if (i > 1) i--;
    }
    else {
      i++;
    }
  }
  return ustr;
}

static void
decompose_hangul(int ucs, int* l, int* v, int* t)
{
  int sindex = ucs - SBASE;
  if (sindex < 0 || sindex >= SCOUNT) {
    *l = ucs;
    *v = *t = 0;
    return;
  }
  *l = LBASE + sindex / NCOUNT;
  *v = VBASE + (sindex % NCOUNT) / TCOUNT;
  *t = TBASE + sindex % TCOUNT;
  if (*t == TBASE) *t = 0;
}

/*
 * push decomposed str into result 
 */
static WString*
decompose_internal(WString* ustr, WString* result)
{
  int i;
  int len = ustr->len;

  for (i = 0; i < len; i++) {
    int ucs = ustr->str[i];
    if (ucs >= SBASE && ucs < SBASE + SCOUNT) {
      int l, v, t;
      decompose_hangul(ucs, &l, &v, &t);
      WStr_addWChar(result, l);
      if (v) WStr_addWChar(result, v);
      if (t) WStr_addWChar(result, t);
    }
    else {
      const char* dc = get_canon(ucs);
      if (!dc) {
	WStr_addWChar(result, ucs);
      }
      else {
	WString wdc;
	WStr_allocWithUTF8(&wdc, dc);
	decompose_internal(&wdc, result);
	WStr_free(&wdc);
      }
    }
  }
  return result;
}

/*
 * push decomposed str into result 
 */
static WString*
decompose_safe_internal(WString* ustr, WString* result)
{
  int i;
  int len = ustr->len;

  for (i = 0; i < len; i++) {
    int ucs = ustr->str[i];
    if (ucs >= SBASE && ucs < SBASE + SCOUNT) {
      int l, v, t;
      decompose_hangul(ucs, &l, &v, &t);
      WStr_addWChar(result, l);
      if (v) WStr_addWChar(result, v);
      if (t) WStr_addWChar(result, t);
    }
    else {
      const char* dc = get_canon_ex(ucs);
      if (!dc) {
	WStr_addWChar(result, ucs);
      }
      else {
	WString wdc;
	WStr_allocWithUTF8(&wdc, dc);
	decompose_safe_internal(&wdc, result);
	WStr_free(&wdc);
      }
    }
  }
  return result;
}

/*
 * push compatibility decomposed str into result 
 */
static WString*
decompose_compat_internal(WString* ustr, WString* result)
{
  int i;
  int len = ustr->len;

  for (i = 0; i < len; i++) {
    int ucs = ustr->str[i];
    if (ucs >= SBASE && ucs < SBASE + SCOUNT) {
      int l, v, t;
      decompose_hangul(ucs, &l, &v, &t);
      WStr_addWChar(result, l);
      if (v) WStr_addWChar(result, v);
      if (t) WStr_addWChar(result, t);
    }
    else {
      const char* dc = get_compat(ucs);
      if (!dc) {
	WStr_addWChar(result, ucs);
      }
      else {
	WString wdc;
	WStr_allocWithUTF8(&wdc, dc);
	decompose_compat_internal(&wdc, result);
	WStr_free(&wdc);
      }
    }
  }
  return result;
}


static int
compose_pair(unsigned int c1, unsigned int c2)
{
  /* Hangul L + V */
  if (c1 >= LBASE && c1 < LBASE + LCOUNT &&
      c2 >= VBASE && c2 < VBASE + VCOUNT) {
    return SBASE + ((c1 - LBASE) * VCOUNT + (c2 - VBASE)) * TCOUNT;
  }
  /* Hangul LV + T */
  else if (c1 >= SBASE && c1 < SBASE + SCOUNT &&
	   (c1 - SBASE) % TCOUNT == 0 &&
	   c2 >= TBASE && c2 < TBASE + TCOUNT) {
    return c1 + (c2 - TBASE);
  }
  return get_composition((int)c1, (int)c2);
}

/*
 * push canonical composed str into result 
 */
static WString*
compose_internal(WString* ustr, WString* result)
{
  int starterPos = 0;
  int starterCh = ustr->str[0];
  int compPos = 1;
  int lastClass = get_cc(starterCh);
  int oldLen = ustr->len;
  int decompPos;

  if (oldLen == 0) return result;
  if (lastClass != 0) lastClass = 256;
  /* copy string */
  result->len = 0;
  WStr_pushWString(result, ustr);

  for (decompPos = compPos; decompPos < result->len; decompPos++) {
    int ch = result->str[decompPos];
    int chClass = get_cc(ch);
    int composite = compose_pair(starterCh, ch);
    if (composite > 0 && 
        (lastClass < chClass ||lastClass == 0)) {
      result->str[starterPos] = composite;
      starterCh = composite;
    }
    else {
      if (chClass == 0) {
        starterPos = compPos;
        starterCh = ch;
      }
      lastClass = chClass;
      result->str[compPos] = ch;
      if (result->len != oldLen) {
        decompPos += result->len - oldLen;
        oldLen = result->len;
      }
      compPos++;
    }
  }
  result->len = compPos;
  return result;
}
#if 0
static WString*
compose_internal(WString* ustr, WString* result)
{
  int len = ustr->len;
  int starter;
  int startercc;
  int i;

  if (len == 0) return result;

  starter = ustr->str[0];
  startercc = get_cc(starter);
  if (startercc != 0) startercc = 256;
  for (i = 1; i < len; i++) {
    int ch = ustr->str[i];
    int cc = get_cc(ch);
    int composite;

    if (startercc == 0 &&
	(composite = compose_pair(starter, ch)) >= 0) {
      starter = composite;
      startercc = get_cc(composite);
    }
    else {
      WStr_addWChar(result, starter);
      starter = ch;
      startercc = cc;
    }
  }
  WStr_addWChar(result, starter);

  return result;
}
#endif

static WString*
upcase_internal(WString* str, WString* result)
{
  int i;
  int len = str->len;

  for (i = 0; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_uppercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      WString wc;
      WStr_allocWithUTF8(&wc, c);
      WStr_pushWString(result, &wc);
      WStr_free(&wc);
    }
  }
  return result;
}

static WString*
downcase_internal(WString* str, WString* result)
{
  int i;
  int len = str->len;

  for (i = 0; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_lowercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      WString wc;
      WStr_allocWithUTF8(&wc, c);
      WStr_pushWString(result, &wc);
      WStr_free(&wc);
    }
  }
  return result;
}

static WString*
capitalize_internal(WString* str,  WString* result)
{
  int i;
  int len = str->len;

  if (len > 0) {
    const char* c = get_titlecase(str->str[0]);
    if (!c) {
      WStr_addWChar(result, str->str[0]);
    }
    else {
      WString wc;
      WStr_allocWithUTF8(&wc, c);
      WStr_pushWString(result, &wc);
      WStr_free(&wc);
    }
  }
  for (i = 1; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_lowercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      WString wc;
      WStr_allocWithUTF8(&wc, c);
      WStr_pushWString(result, &wc);
      WStr_free(&wc);
    }
  }
  return result;
}

/* Codepoint order is UTF-8 byte order, so this orders strings the way comparing their UTF-8 bytes does. */
static int
compare_codepoints(const WString* a, const WString* b)
{
  int i;
  int shorter = a->len < b->len ? a->len : b->len;

  for (i = 0; i < shorter; i++) {
    if (a->str[i] != b->str[i])
      return a->str[i] < b->str[i] ? -1 : 1;
  }
  if (a->len == b->len)
    return 0;
  return a->len < b->len ? -1 : 1;
}

/*
 * Compares the canonically ordered decompositions of two strings, returning -1, 0, or 1. Only
 * utf8_argument can raise, and it runs before any buffer is allocated; nothing after it calls the
 * Ruby API, so the buffers are always freed.
 */
static VALUE
compare_decompositions(VALUE str1, VALUE str2, WString* (*decompose)(WString*, WString*))
{
  WString wstr1;
  WString wstr2;
  WString result1;
  WString result2;
  int order;

  str1 = utf8_argument(str1);
  str2 = utf8_argument(str2);
  WStr_allocWithUTF8L(&wstr1, RSTRING_PTR(str1), RSTRING_LEN(str1));
  WStr_allocWithUTF8L(&wstr2, RSTRING_PTR(str2), RSTRING_LEN(str2));
  WStr_alloc(&result1);
  WStr_alloc(&result2);
  decompose(&wstr1, &result1);
  decompose(&wstr2, &result2);
  WStr_free(&wstr1);
  WStr_free(&wstr2);
  sort_canonical(&result1);
  sort_canonical(&result2);
  order = compare_codepoints(&result1, &result2);
  WStr_free(&result1);
  WStr_free(&result2);

  return INT2FIX(order);
}

static VALUE
unicode_strcmp(VALUE obj, VALUE str1, VALUE str2)
{
  return compare_decompositions(str1, str2, decompose_internal);
}

static VALUE
unicode_strcmp_compat(VALUE obj, VALUE str1, VALUE str2)
{
  return compare_decompositions(str1, str2, decompose_compat_internal);
}

struct transform_steps {
  WString* (*first)(WString*, WString*);
  int sort;
  WString* (*second)(WString*, WString*);
};

static VALUE
transform_string(VALUE str, const struct transform_steps* steps)
{
  WString current;
  WString next;
  UString utf8;
  VALUE result;

  str = utf8_argument(str);
  WStr_allocWithUTF8L(&current, RSTRING_PTR(str), RSTRING_LEN(str));
  if (steps->first) {
    WStr_alloc(&next);
    steps->first(&current, &next);
    WStr_free(&current);
    current = next;
  }
  if (steps->sort)
    sort_canonical(&current);
  if (steps->second) {
    WStr_alloc(&next);
    steps->second(&current, &next);
    WStr_free(&current);
    current = next;
  }
  UniStr_alloc(&utf8);
  WStr_convertIntoUString(&current, &utf8);
  WStr_free(&current);
  result = ENC_(rb_str_new((char*)utf8.str, utf8.len));
  UniStr_free(&utf8);

  return result;
}

static const struct transform_steps decompose_steps = { decompose_internal, 1, NULL };
static const struct transform_steps decompose_safe_steps = { decompose_safe_internal, 1, NULL };
static const struct transform_steps decompose_compat_steps = { decompose_compat_internal, 1, NULL };
static const struct transform_steps compose_steps = { NULL, 1, compose_internal };
static const struct transform_steps normalize_C_steps = { decompose_internal, 1, compose_internal };
static const struct transform_steps normalize_safe_steps = { decompose_safe_internal, 1, compose_internal };
static const struct transform_steps normalize_KC_steps = { decompose_compat_internal, 1, compose_internal };
static const struct transform_steps upcase_steps = { upcase_internal, 0, NULL };
static const struct transform_steps downcase_steps = { downcase_internal, 0, NULL };
static const struct transform_steps capitalize_steps = { capitalize_internal, 0, NULL };

static VALUE
unicode_decompose(VALUE obj, VALUE str)
{
  return transform_string(str, &decompose_steps);
}

static VALUE
unicode_decompose_safe(VALUE obj, VALUE str)
{
  return transform_string(str, &decompose_safe_steps);
}

static VALUE
unicode_decompose_compat(VALUE obj, VALUE str)
{
  return transform_string(str, &decompose_compat_steps);
}

static VALUE
unicode_compose(VALUE obj, VALUE str)
{
  return transform_string(str, &compose_steps);
}

static VALUE
unicode_normalize_C(VALUE obj, VALUE str)
{
  return transform_string(str, &normalize_C_steps);
}

static VALUE
unicode_normalize_safe(VALUE obj, VALUE str)
{
  return transform_string(str, &normalize_safe_steps);
}

static VALUE
unicode_normalize_KC(VALUE obj, VALUE str)
{
  return transform_string(str, &normalize_KC_steps);
}

static VALUE
unicode_upcase(VALUE obj, VALUE str)
{
  return transform_string(str, &upcase_steps);
}

static VALUE
unicode_downcase(VALUE obj, VALUE str)
{
  return transform_string(str, &downcase_steps);
}

static VALUE
unicode_capitalize(VALUE obj, VALUE str)
{
  return transform_string(str, &capitalize_steps);
}

typedef struct _get_categories_param {
  WString* wstr;
  VALUE str;
  VALUE* catname;
} get_categories_param;

static VALUE
get_categories_internal(VALUE param_value)
{
  get_categories_param* param = (get_categories_param*)param_value;
  WString* wstr = param->wstr;
  VALUE str = param->str;
  VALUE* catname = param->catname;
  int pos;
  int block_p = rb_block_given_p();
  volatile VALUE ret = str;

  if (!block_p)
    ret = rb_ary_new();
  for (pos = 0; pos < wstr->len; pos++) {
    int gencat = get_gencat(wstr->str[pos]);
    if (!block_p)
      rb_ary_push(ret, catname[gencat]);
    else {
      rb_yield(catname[gencat]);
    }
  }
 
  return ret;
}

VALUE
get_categories_ensure(VALUE wstr_value)
{
  WString* wstr = (WString*)wstr_value;
  WStr_free(wstr);
  return Qnil;
}

VALUE
unicode_get_categories(VALUE obj, VALUE str)
{
  WString wstr;
  get_categories_param param = { &wstr, str, catname_long };

  str = utf8_argument(str);
  WStr_allocWithUTF8L(&wstr, RSTRING_PTR(str), RSTRING_LEN(str));

  return rb_ensure(get_categories_internal, (VALUE)&param,
                   get_categories_ensure, (VALUE)&wstr);
  /* wstr will be freed in get_text_elements_ensure() */
}


VALUE
unicode_get_abbr_categories(VALUE obj, VALUE str)
{
  WString wstr;
  get_categories_param param = { &wstr, str, catname_abbr };

  str = utf8_argument(str);
  WStr_allocWithUTF8L(&wstr, RSTRING_PTR(str), RSTRING_LEN(str));

  return rb_ensure(get_categories_internal, (VALUE)&param,
                   get_categories_ensure, (VALUE)&wstr);
  /* wstr will be freed in get_text_elements_ensure() */
}

VALUE
unicode_wcswidth(int argc, VALUE* argv, VALUE obj)
{
  WString wstr;
  int i, count;
  int width = 0;
  int cjk_p = 0;
  VALUE str;
  VALUE cjk;

  count = rb_scan_args(argc, argv, "11", &str, &cjk);
  if (count > 1)
    cjk_p = RTEST(cjk);
  str = utf8_argument(str);
  WStr_allocWithUTF8L(&wstr, RSTRING_PTR(str), RSTRING_LEN(str));
  for (i = 0; i <wstr.len; i++) {
    int c = wstr.str[i];
    int cat = get_gencat(c);
    int eaw = get_eawidth(c);
    if ((c > 0 && c < 32) || (c >= 0x7f && c < 0xa0)) {
      /* Control Characters */
      width = -1;
      break;
    }
    else if (c != 0x00ad && /* SOFT HYPHEN */
             (cat == c_Mn || cat == c_Me || /* Non-spacing Marks */
              cat == c_Cf || /* Format */
              c == 0 || /* NUL */
              (c >= 0x1160 && c <= 0x11ff))) /* HANGUL JUNGSEONG/JONGSEONG */
      /* zero width */ ;
    else if (eaw == w_F || eaw == w_W || /* Fullwidth or Wide */
             (c >= 0x4db6 && c <= 0x4dbf) || /* CJK Reserved */
             (c >= 0x9fcd && c <= 0x9fff) || /* CJK Reserved */
             (c >= 0xfa6e && c <= 0xfa6f) || /* CJK Reserved */
             (c >= 0xfada && c <= 0xfaff) || /* CJK Reserved */
             (c >= 0x2a6d7 && c <= 0x2a6ff) || /* CJK Reserved */
             (c >= 0x2b735 && c <= 0x2b73f) || /* CJK Reserved */
             (c >= 0x2b81e && c <= 0x2f7ff) || /* CJK Reserved */
             (c >= 0x2fa1e && c <= 0x2fffd) || /* CJK Reserved */
             (c >= 0x30000 && c <= 0x3fffd) || /* CJK Reserved */
             (cjk_p && eaw == w_A)) /* East Asian Ambiguous */
      width += 2;
    else
      width++; /* Halfwidth or Neutral */
  }
  WStr_free(&wstr);

  return INT2FIX(width);
}

VALUE
wstring_to_rstring(WString* wstr, int start, int len) {
  UString ret;
  volatile VALUE vret;

  UniStr_alloc(&ret);
  WStr_convertIntoUString2(wstr, start, len, &ret);
  vret = ENC_(rb_str_new((char*)ret.str, ret.len));
  UniStr_free(&ret);

  return vret;
}

typedef struct _get_text_elements_param {
  WString* wstr;
  VALUE str;
} get_text_elements_param;

VALUE
get_text_elements_internal(VALUE param_value)
{
  get_text_elements_param* param = (get_text_elements_param*)param_value;
  WString* wstr = param->wstr;
  VALUE str = param->str;
  int start_pos;
  int block_p = rb_block_given_p();
  volatile VALUE ret = str;

  if (!block_p)
    ret = rb_ary_new();
  for (start_pos = 0; start_pos < wstr->len;) {
    int c0 = wstr->str[start_pos];
    int cat = get_gencat(c0);
    int length = 1;
    int j;

    if (cat == c_Mn || cat == c_Mc || cat == c_Me) {
      volatile VALUE rstr = TO_(str, wstring_to_rstring(wstr, start_pos, length));
      if (!block_p)
        rb_ary_push(ret, rstr);
      else
        rb_yield(rstr);
      start_pos++;
      continue;
    }

    for (j = start_pos + 1; j < wstr->len; j++) {
      int c1 = wstr->str[j];
      int cat = get_gencat(c1);
      if (c0 >= LBASE && c0 < LBASE + LCOUNT &&
          j + 1 < wstr->len &&
          c1 >= VBASE && c1 < VBASE + VCOUNT &&
          wstr->str[j+1] >= TBASE && wstr->str[j+1] < TBASE + TCOUNT) {
        /* Hangul L+V+T */
        length += 2;
        j++;
      }
      else if (c0 >= LBASE && c0 < LBASE + LCOUNT &&
               c1 >= VBASE && c1< VBASE + VCOUNT) {
        /* Hangul L+V */
        length++;
      }
      else if (c0 >= SBASE && c0 < SBASE + SCOUNT &&
               (c0 - SBASE) % TCOUNT == 0 &&
               c1 >= TBASE && c1 < TBASE + TCOUNT) {
        /* Hangul LV+T */
        length++;
      }
      else if (cat == c_Mn || cat == c_Mc || cat == c_Me) {
        /* Mark */
        length++;
      }
      else {
        volatile VALUE rstr = TO_(str, wstring_to_rstring(wstr, start_pos, length));
        if (!block_p)
          rb_ary_push(ret, rstr);
        else
          rb_yield(rstr);
        length = 0;
        break;
      }
    }
    if (length > 0) {
      volatile VALUE rstr = TO_(str, wstring_to_rstring(wstr, start_pos, length));
      if (!block_p)
        rb_ary_push(ret, rstr);
      else
        rb_yield(rstr);
    }
    start_pos = j;
  }
  return ret;
}

VALUE
get_text_elements_ensure(VALUE wstr_value)
{
  WString* wstr = (WString*)wstr_value;
  WStr_free(wstr);
  return Qnil;
}

VALUE
unicode_get_text_elements(VALUE obj, VALUE str)
{
  WString wstr;
  get_text_elements_param param = { &wstr, str };

  str = utf8_argument(str);
  WStr_allocWithUTF8L(&wstr, RSTRING_PTR(str), RSTRING_LEN(str));

  return rb_ensure(get_text_elements_internal, (VALUE)&param,
                   get_text_elements_ensure, (VALUE)&wstr);
  /* wstr will be freed in get_text_elements_ensure() */
}

void
Init_unicode_native()
{
  int i;

#ifdef HAVE_RUBY_ENCODING_H
  enc_out = rb_utf8_encoding();
#endif

  mUnicode = rb_define_module("Unicode");
  build_bmp_index();
  build_composition_pairs();

  for (i = 0; i < c_Cn + 1; i++) {
    catname_abbr[i] = ID2SYM(rb_intern(gencat_abbr[i]));
    catname_long[i] = ID2SYM(rb_intern(gencat_long[i]));
    rb_global_variable(&catname_abbr[i]);
    rb_global_variable(&catname_long[i]);
  }

  rb_define_module_function(mUnicode, "strcmp",
			    unicode_strcmp, 2);
  rb_define_module_function(mUnicode, "strcmp_compat",
			    unicode_strcmp_compat, 2);

  rb_define_module_function(mUnicode, "decompose",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "decompose_safe",
			    unicode_decompose_safe, 1);
  rb_define_module_function(mUnicode, "decompose_compat",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "compose",
			    unicode_compose, 1);

  rb_define_module_function(mUnicode, "normalize_D",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "normalize_D_safe",
			    unicode_decompose_safe, 1);
  rb_define_module_function(mUnicode, "normalize_KD",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "normalize_C",
			    unicode_normalize_C, 1);
  rb_define_module_function(mUnicode, "normalize_C_safe",
			    unicode_normalize_safe, 1);
  rb_define_module_function(mUnicode, "normalize_KC",
			    unicode_normalize_KC, 1);

  /* aliases */
  rb_define_module_function(mUnicode, "nfd",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "nfd_safe",
			    unicode_decompose_safe, 1);
  rb_define_module_function(mUnicode, "nfkd",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "nfc",
			    unicode_normalize_C, 1);
  rb_define_module_function(mUnicode, "nfc_safe",
			    unicode_normalize_safe, 1);
  rb_define_module_function(mUnicode, "nfkc",
			    unicode_normalize_KC, 1);

  rb_define_module_function(mUnicode, "upcase",
			    unicode_upcase, 1);
  rb_define_module_function(mUnicode, "downcase",
			    unicode_downcase, 1);
  rb_define_module_function(mUnicode, "capitalize",
			    unicode_capitalize, 1);

  rb_define_module_function(mUnicode, "categories",
			    unicode_get_categories, 1);
  rb_define_module_function(mUnicode, "abbr_categories",
			    unicode_get_abbr_categories, 1);
  rb_define_module_function(mUnicode, "width",
			    unicode_wcswidth, -1);
  rb_define_module_function(mUnicode, "text_elements",
			    unicode_get_text_elements, 1);
}
