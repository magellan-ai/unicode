/*
 * Unicode normalization, case mapping, and character properties for Ruby.
 * Original library by Yoshida Masato, 1999-2013. See LICENSE.
 */

#include "ruby.h"
#include "ruby/encoding.h"
#include <stdlib.h>
#include "wstring.h"
#include "allocation.h"
#include "unidata.map"

static rb_encoding* enc_out;
#define ENC_(o) (rb_enc_associate(o, enc_out))

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
 * Mappings (decompositions and case mappings in unidata.map) are decoded into stack arrays of this
 * many codepoints, so helpers never heap-allocate a temporary that a raise could leak. The longest
 * mapping in the Unicode 6.2 data is 18 codepoints (the compatibility decomposition of U+FDFA);
 * check_mapping_lengths refuses to load data with one longer than this.
 */
#define MAPPING_CAPACITY 32

static int
decode_mapping(const char* utf8, int codepoints[MAPPING_CAPACITY])
{
  const unsigned char* p = (const unsigned char*)utf8;
  int count = 0;

  while (*p && count < MAPPING_CAPACITY) {
    int codepoint;
    int continuation_bytes;

    if (*p < 0x80) {
      codepoint = *p;
      continuation_bytes = 0;
    }
    else if (*p < 0xE0) {
      codepoint = *p & 0x1F;
      continuation_bytes = 1;
    }
    else if (*p < 0xF0) {
      codepoint = *p & 0x0F;
      continuation_bytes = 2;
    }
    else {
      codepoint = *p & 0x07;
      continuation_bytes = 3;
    }
    p++;
    while (continuation_bytes-- > 0 && *p)
      codepoint = (codepoint << 6) | (*p++ & 0x3F);
    codepoints[count++] = codepoint;
  }
  return count;
}

static int
mapping_length(const char* utf8)
{
  const unsigned char* p;
  int length = 0;

  for (p = (const unsigned char*)utf8; p && *p; p++) {
    if ((*p & 0xC0) != 0x80)
      length++;
  }
  return length;
}

static void
check_mapping_lengths(void)
{
  size_t i;

  for (i = 0; i < UNIDATA_COUNT; i++) {
    const char* mappings[] = {
      unidata[i].canon, unidata[i].compat, unidata[i].uppercase, unidata[i].lowercase, unidata[i].titlecase
    };
    size_t m;

    for (m = 0; m < sizeof(mappings) / sizeof(mappings[0]); m++) {
      if (mapping_length(mappings[m]) > MAPPING_CAPACITY)
        rb_raise(rb_eRuntimeError, "unidata.map maps U+%04X to more than %d codepoints", unidata[i].code, MAPPING_CAPACITY);
    }
  }
}

static void
append_mapping(WString* result, const char* utf8)
{
  int mapping[MAPPING_CAPACITY];

  WStr_addWChars(result, mapping, decode_mapping(utf8, mapping));
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
    int decomposition[MAPPING_CAPACITY];

    if (!unidata[i].canon || unidata[i].exclusion)
      continue;
    if (decode_mapping(unidata[i].canon, decomposition) == 2) {
      composition_pairs[composition_pair_count].first = decomposition[0];
      composition_pairs[composition_pair_count].second = decomposition[1];
      composition_pairs[composition_pair_count].composite = unidata[i].code;
      composition_pair_count++;
    }
  }
  qsort(composition_pairs, composition_pair_count, sizeof(struct composition_pair), compare_composition_pairs);
}

static WString*
sort_canonical(WString* ustr)
{
  long i = 1;
  long len = ustr->len;

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
  long i;
  long len = ustr->len;

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
	int mapping[MAPPING_CAPACITY];
	WString decomposition = { .str = mapping, .len = decode_mapping(dc, mapping), .size = MAPPING_CAPACITY };

	decompose_internal(&decomposition, result);
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
  long i;
  long len = ustr->len;

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
	int mapping[MAPPING_CAPACITY];
	WString decomposition = { .str = mapping, .len = decode_mapping(dc, mapping), .size = MAPPING_CAPACITY };

	decompose_safe_internal(&decomposition, result);
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
  long i;
  long len = ustr->len;

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
	int mapping[MAPPING_CAPACITY];
	WString decomposition = { .str = mapping, .len = decode_mapping(dc, mapping), .size = MAPPING_CAPACITY };

	decompose_compat_internal(&decomposition, result);
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
  long starterPos = 0;
  int starterCh;
  long compPos = 1;
  int lastClass;
  long oldLen = ustr->len;
  long decompPos;

  if (oldLen == 0) return result;
  starterCh = ustr->str[0];
  lastClass = get_cc(starterCh);
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

static WString*
upcase_internal(WString* str, WString* result)
{
  long i;
  long len = str->len;

  for (i = 0; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_uppercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      append_mapping(result, c);
    }
  }
  return result;
}

static WString*
downcase_internal(WString* str, WString* result)
{
  long i;
  long len = str->len;

  for (i = 0; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_lowercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      append_mapping(result, c);
    }
  }
  return result;
}

static WString*
capitalize_internal(WString* str,  WString* result)
{
  long i;
  long len = str->len;

  if (len > 0) {
    const char* c = get_titlecase(str->str[0]);
    if (!c) {
      WStr_addWChar(result, str->str[0]);
    }
    else {
      append_mapping(result, c);
    }
  }
  for (i = 1; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_lowercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      append_mapping(result, c);
    }
  }
  return result;
}

struct transform_steps {
  WString* (*first)(WString*, WString*);
  int sort;
  WString* (*second)(WString*, WString*);
};

struct call {
  VALUE string;
  VALUE other_string;
  VALUE original;
  const struct transform_steps* steps;
  const VALUE* category_names;
  int cjk;
  WString wide[6];
  UString utf8;
};

static VALUE
free_call_buffers(VALUE argument)
{
  struct call* call = (struct call*)argument;
  size_t i;

  for (i = 0; i < sizeof(call->wide) / sizeof(call->wide[0]); i++)
    WStr_free(&call->wide[i]);
  UniStr_free(&call->utf8);
  return Qnil;
}

static VALUE
call_with_buffers(VALUE (*work)(VALUE), struct call* call)
{
  return rb_ensure(work, (VALUE)call, free_call_buffers, (VALUE)call);
}

static WString*
decode_into(VALUE string, WString* buffer)
{
  WStr_allocWithUTF8L(buffer, RSTRING_PTR(string), RSTRING_LEN(string));
  return buffer;
}

static WString*
apply_step(WString* (*step)(WString*, WString*), WString* input, WString* output)
{
  WStr_alloc(output);
  step(input, output);
  WStr_free(input);
  return output;
}

/* Decodes string into slots[0] and applies steps, putting each step's result in the next slot. */
static WString*
transform_into(VALUE string, const struct transform_steps* steps, WString slots[3])
{
  WString* current = decode_into(string, &slots[0]);

  if (steps->first)
    current = apply_step(steps->first, current, &slots[1]);
  if (steps->sort)
    sort_canonical(current);
  if (steps->second)
    current = apply_step(steps->second, current, &slots[2]);
  return current;
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

/* Codepoint order is UTF-8 byte order, so this orders strings the way comparing their UTF-8 bytes does. */
static int
compare_codepoints(const WString* a, const WString* b)
{
  long i;
  long shorter = a->len < b->len ? a->len : b->len;

  for (i = 0; i < shorter; i++) {
    if (a->str[i] != b->str[i])
      return a->str[i] < b->str[i] ? -1 : 1;
  }
  if (a->len == b->len)
    return 0;
  return a->len < b->len ? -1 : 1;
}

static VALUE
compare_work(VALUE argument)
{
  struct call* call = (struct call*)argument;
  const WString* left;
  const WString* right;

  call->string = utf8_argument(call->string);
  call->other_string = utf8_argument(call->other_string);
  left = transform_into(call->string, call->steps, &call->wide[0]);
  right = transform_into(call->other_string, call->steps, &call->wide[3]);
  return INT2FIX(compare_codepoints(left, right));
}

/* Compares the canonically ordered decompositions of two strings, returning -1, 0, or 1. */
static VALUE
compare_decompositions(VALUE str1, VALUE str2, const struct transform_steps* decompose)
{
  struct call call = { .string = str1, .other_string = str2, .steps = decompose };

  return call_with_buffers(compare_work, &call);
}

static VALUE
unicode_strcmp(VALUE obj, VALUE str1, VALUE str2)
{
  return compare_decompositions(str1, str2, &decompose_steps);
}

static VALUE
unicode_strcmp_compat(VALUE obj, VALUE str1, VALUE str2)
{
  return compare_decompositions(str1, str2, &decompose_compat_steps);
}

static VALUE
transform_work(VALUE argument)
{
  struct call* call = (struct call*)argument;
  const WString* result;

  call->string = utf8_argument(call->string);
  result = transform_into(call->string, call->steps, call->wide);
  UniStr_alloc(&call->utf8);
  WStr_convertIntoUString((WString*)result, &call->utf8);
  return ENC_(rb_str_new((char*)call->utf8.str, call->utf8.len));
}

static VALUE
transform_string(VALUE str, const struct transform_steps* steps)
{
  struct call call = { .string = str, .steps = steps };

  return call_with_buffers(transform_work, &call);
}

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

static void
deliver(VALUE elements, int block_given, VALUE element)
{
  if (block_given)
    rb_yield(element);
  else
    rb_ary_push(elements, element);
}

static VALUE
categories_work(VALUE argument)
{
  struct call* call = (struct call*)argument;
  int block_given = rb_block_given_p();
  const WString* text;
  VALUE categories;
  long i;

  call->string = utf8_argument(call->string);
  text = decode_into(call->string, &call->wide[0]);
  categories = block_given ? call->original : rb_ary_new();
  for (i = 0; i < text->len; i++)
    deliver(categories, block_given, call->category_names[get_gencat(text->str[i])]);
  return categories;
}

static VALUE
unicode_get_categories(VALUE obj, VALUE str)
{
  struct call call = { .string = str, .original = str, .category_names = catname_long };

  return call_with_buffers(categories_work, &call);
}

static VALUE
unicode_get_abbr_categories(VALUE obj, VALUE str)
{
  struct call call = { .string = str, .original = str, .category_names = catname_abbr };

  return call_with_buffers(categories_work, &call);
}

static VALUE
width_work(VALUE argument)
{
  struct call* call = (struct call*)argument;
  const WString* text;
  long width = 0;
  long i;

  call->string = utf8_argument(call->string);
  text = decode_into(call->string, &call->wide[0]);
  for (i = 0; i < text->len; i++) {
    int c = text->str[i];
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
             (call->cjk && eaw == w_A)) /* East Asian Ambiguous */
      width += 2;
    else
      width++; /* Halfwidth or Neutral */
  }
  return LONG2NUM(width);
}

static VALUE
unicode_wcswidth(int argc, VALUE* argv, VALUE obj)
{
  VALUE str;
  VALUE cjk;
  struct call call = { .cjk = 0 };

  rb_scan_args(argc, argv, "11", &str, &cjk);
  call.string = str;
  call.cjk = RTEST(cjk);
  return call_with_buffers(width_work, &call);
}

static VALUE
element_string(struct call* call, long start, long length)
{
  call->utf8.len = 0;
  WStr_convertIntoUString2(&call->wide[0], start, length, &call->utf8);
  return ENC_(rb_str_new((char*)call->utf8.str, call->utf8.len));
}

static VALUE
text_elements_work(VALUE argument)
{
  struct call* call = (struct call*)argument;
  int block_given = rb_block_given_p();
  const WString* wstr;
  VALUE elements;
  long start_pos;

  call->string = utf8_argument(call->string);
  wstr = decode_into(call->string, &call->wide[0]);
  UniStr_alloc(&call->utf8);
  elements = block_given ? call->original : rb_ary_new();
  for (start_pos = 0; start_pos < wstr->len;) {
    int c0 = wstr->str[start_pos];
    int cat = get_gencat(c0);
    long length = 1;
    long j;

    if (cat == c_Mn || cat == c_Mc || cat == c_Me) {
      deliver(elements, block_given, element_string(call, start_pos, length));
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
        deliver(elements, block_given, element_string(call, start_pos, length));
        length = 0;
        break;
      }
    }
    if (length > 0)
      deliver(elements, block_given, element_string(call, start_pos, length));
    start_pos = j;
  }
  return elements;
}

static VALUE
unicode_get_text_elements(VALUE obj, VALUE str)
{
  struct call call = { .string = str, .original = str };

  return call_with_buffers(text_elements_work, &call);
}

void
Init_unicode_native(void)
{
  int i;

  enc_out = rb_utf8_encoding();

  mUnicode = rb_define_module("Unicode");
#ifdef UNICODE_FAULT_INJECTION
  Init_unicode_fault_injection(mUnicode);
#endif
  check_mapping_lengths();
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
