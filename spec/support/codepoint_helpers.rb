# frozen_string_literal: true

module CodepointHelpers
  SURROGATES = (0xD800..0xDFFF)

  class << self
    def every_character
      @every_character ||= (0..0x10FFFF).filter_map { |codepoint| codepoint.chr(Encoding::UTF_8) unless SURROGATES.cover?(codepoint) }.freeze
    end

    def unassigned_in_unicode_6_2
      @unassigned_in_unicode_6_2 ||= every_character.select { |char| Unicode.abbr_categories(char) == [:Cn] }.to_set.freeze
    end
  end

  def every_character
    CodepointHelpers.every_character
  end

  def assigned_in_unicode_6_2?(char)
    !CodepointHelpers.unassigned_in_unicode_6_2.include?(char)
  end

  def codepoint_label(string)
    string.codepoints.map { |codepoint| format('U+%<codepoint>04X', codepoint:) }.join(' ')
  end
end
