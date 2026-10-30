# frozen_string_literal: true

MAP_HEADER = <<~C
  /*
   * UnicodeData
   * Copyright 1999, 2004, 2010, 2012 by yoshidam
   *
   */

  #ifndef _UNIDATA_MAP
  #define _UNIDATA_MAP

C

MAP_DECLARATIONS = <<~C

  enum GeneralCategory {
    /* Letter */
    c_Lu = 1, c_Ll, c_Lt, c_LC, c_Lm, c_Lo,
    /* Mark */
    c_Mn, c_Mc, c_Me,
    /* Number */
    c_Nd, c_Nl, c_No,
    /* Punctuation */
    c_Pc, c_Pd, c_Ps, c_Pe, c_Pi, c_Pf, c_Po,
    /* Symbol */
    c_Sm, c_Sc, c_Sk, c_So,
    /* Separator */
    c_Zs, c_Zl, c_Zp,
    /* Other */
    c_Cc, c_Cf, c_Cs, c_Co, c_Cn
  };

  const char* const gencat_abbr[] = {
    "", /* 0 */
    /* Letter */
    "Lu", "Ll", "Lt", "LC", "Lm", "Lo",
    /* Mark */
    "Mn", "Mc", "Me",
    /* Number */
    "Nd", "Nl", "No",
    /* Punctuation */
    "Pc", "Pd", "Ps", "Pe", "Pi", "Pf", "Po",
    /* Symbol */
    "Sm", "Sc", "Sk", "So",
    /* Separator */
    "Zs", "Zl", "Zp",
    /* Other */
    "Cc", "Cf", "Cs", "Co", "Cn"
  };

  const char* const gencat_long[] = {
    "",
    "Uppercase_Letter",
    "Lowercase_Letter",
    "Titlecase_Letter",
    "Cased_Letter",
    "Modifier_Letter",
    "Other_Letter",
    "Nonspacing_Mark",
    "Spacing_Mark",
    "Enclosing_Mark",
    "Decimal_Number",
    "Letter_Number",
    "Other_Number",
    "Connector_Punctuation",
    "Dash_Punctuation",
    "Open_Punctuation",
    "Close_Punctuation",
    "Initial_Punctuation",
    "Final_Punctuation",
    "Other_Punctuation",
    "Math_Symbol",
    "Currency_Symbol",
    "Modifier_Symbol",
    "Other_Symbol",
    "Space_Separator",
    "Line_Separator",
    "Paragraph_Separator",
    "Control",
    "Format",
    "Surrogate",
    "Private_Use",
    "Unassigned"
  };

  enum EastAsianWidth {
    w_N = 1, w_A, w_H, w_W, w_F, w_Na
  };

  struct unicode_data {
    const int code;
    const char* const canon;
    const char* const compat;
    const char* const uppercase;
    const char* const lowercase;
    const char* const titlecase;
    const unsigned char combining_class;
    const unsigned char exclusion;
    const unsigned char general_category;
    const unsigned char east_asian_width;
  };

  static const struct unicode_data unidata[] = {
C

MAP_FOOTER = <<~C
  };

  #endif
C

NOT_EXCLUDED = 0
FULL_COMPOSITION_EXCLUSION = 1
SINGLETON_DECOMPOSITION = 2
NON_STARTER_DECOMPOSITION = 3

def generate_unidata_map(unicode_data_path, normalization_props_path, special_casing_path, east_asian_width_path)
  exclusions = composition_exclusions(normalization_props_path)
  casings = special_casings(special_casing_path)
  characters, ranges = unicode_data(unicode_data_path)
  widths = east_asian_widths(east_asian_width_path)

  print MAP_HEADER
  ranges.each { |name, code| printf("#define %s\t(0x%04x)\n", name, code) }
  print MAP_DECLARATIONS
  characters.sort.each { |code, data| print unidata_entry(code, data, characters, exclusions, casings, widths) }
  printf("  { -1, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 }\n")
  print MAP_FOOTER
end

def composition_exclusions(path)
  data_lines(path).grep(/Full_Composition_Exclusion/).flat_map { |line| codepoint_range(line.split.first).to_a }.to_set
end

def data_lines(path)
  File.foreach(path, chomp: true).reject { |line| line.start_with?('#') || line.empty? }
end

def codepoint_range(field)
  first, last = field.split('..')
  first.hex..(last || first).hex
end

def special_casings(path)
  data_lines(path).each_with_object({}) do |line, casings|
    code, lower, title, upper, condition = line[/^(.*)#\s*(.*)$/, 1].split(/;\s/)
    next if condition

    casings[code.hex] = [lower, title, upper].map { |mapping| optional_string(mapping) unless mapping == code }
  end
end

def optional_string(field)
  string_from_hex(field.split) unless field.nil? || field.empty?
end

def string_from_hex(hex_codepoints)
  hex_codepoints.map(&:hex).pack('U*')
end

def unicode_data(path)
  characters = {}
  ranges = []
  data_lines(path).each do |line|
    code, name, category, combining_class, _, decomposition, _, _, _, _, _, _, uppercase, lowercase, titlecase =
      line.split(';', 15)
    canonical, compatibility = decompositions(decomposition)
    characters[code.hex] = [
      combining_class.to_i, canonical, compatibility,
      optional_string(uppercase), optional_string(lowercase), optional_string(titlecase),
      category
    ]
    range_name = name[/^<(.*, (First|Last))>$/, 1]
    ranges << [range_name.upcase.gsub(/,? /, '_'), code.hex] if range_name
  end
  [characters, ranges]
end

def decompositions(field)
  return [nil, nil] if field.nil? || field.empty?

  mapping = field.split
  if mapping.first.match?(/^[0-9A-F]{4,6}$/)
    canonical = string_from_hex(mapping)
    [canonical, canonical]
  elsif mapping.first.match?(/^<.+>$/)
    [nil, string_from_hex(mapping.drop(1))]
  else
    raise "unknown value: #{field}"
  end
end

def east_asian_widths(path)
  data_lines(path).each_with_object({}) do |line, widths|
    codes, width = line[/^(.*)\s+#\s*(.*)$/, 1].split(';')
    codepoint_range(codes).each { |code| widths[code] = width }
  end
end

def unidata_entry(code, data, characters, exclusions, casings, widths)
  combining_class, canonical, compatibility, uppercase, lowercase, titlecase, category = data
  special_lower, special_title, special_upper = casings[code]

  format(
    "  { 0x%04x, %s, %s, %s, %s, %s, %d, %d, c_%s, w_%s }, \n",
    code,
    c_string_literal(canonical),
    c_string_literal(compatibility),
    c_string_literal(special_upper || uppercase),
    c_string_literal(special_lower || lowercase),
    c_string_literal(special_title || titlecase),
    combining_class,
    exclusion_kind(code, canonical, characters, exclusions),
    category,
    widths.fetch(code, 'N')
  )
end

def c_string_literal(string)
  return 'NULL' unless string

  escaped = string.each_byte.map { |byte| printable_ascii?(byte) ? byte.chr : format('\\%03o', byte) }.join
  %("#{escaped}")
end

def printable_ascii?(byte)
  byte.between?(32, 126) && byte != '"'.ord && byte != '\\'.ord
end

def exclusion_kind(code, canonical, characters, exclusions)
  if exclusions.include?(code)
    FULL_COMPOSITION_EXCLUSION
  elsif canonical&.length == 1
    SINGLETON_DECOMPOSITION
  elsif canonical && characters[canonical.ord].first != 0
    NON_STARTER_DECOMPOSITION
  else
    NOT_EXCLUDED
  end
end

abort "Usage: ruby #{$PROGRAM_NAME} <UnicodeData.txt> <DerivedNormalizationProps.txt> <SpecialCasing.txt> <EastAsianWidth.txt>" unless ARGV.length == 4

generate_unidata_map(*ARGV)
