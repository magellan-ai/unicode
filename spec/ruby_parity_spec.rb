# frozen_string_literal: true

# This extension is pinned to Unicode 6.2; Ruby's mappings are newer. Post-6.2 chars are
# unchanged, as are the few 6.2 characters whose case mappings were only added later.
RSpec.describe Unicode do
  let(:cherokee) { 0x13A0..0x13F4 }
  let(:georgian_mkhedruli) { [*0x10D0..0x10FA, *0x10FD..0x10FF] }
  let(:latin_and_greek_with_later_capitals) do
    [0x019B, 0x025C, 0x0261, 0x0264, 0x026A, 0x026C, 0x0282, 0x0287, 0x029D, 0x029E, 0x03F3, 0x1D8E]
  end

  it 'matches String#downcase, except for Cherokee, which gained lowercase letters later' do
    differences =
      every_character.reject do |char|
        unchanged = !assigned_in_unicode_6_2?(char) || cherokee.cover?(char.ord)
        described_class.downcase(char) == (unchanged ? char : char.downcase)
      end

    expect(differences.map { |char| codepoint_label(char) }).to eq([])
  end

  it 'matches String#upcase, except for letters that gained capitals later' do
    later_capitals = [*georgian_mkhedruli, *latin_and_greek_with_later_capitals].to_set
    differences =
      every_character.reject do |char|
        unchanged = !assigned_in_unicode_6_2?(char) || later_capitals.include?(char.ord)
        described_class.upcase(char) == (unchanged ? char : char.upcase)
      end

    expect(differences.map { |char| codepoint_label(char) }).to eq([])
  end

  it 'matches String#capitalize, except for letters that gained titlecase forms later' do
    later_titlecase = latin_and_greek_with_later_capitals.to_set
    differences =
      every_character.reject do |char|
        unchanged = !assigned_in_unicode_6_2?(char) || later_titlecase.include?(char.ord)
        described_class.capitalize(char) == (unchanged ? char : char.capitalize)
      end

    expect(differences.map { |char| codepoint_label(char) }).to eq([])
  end

  it 'matches String#unicode_normalize in every form' do
    forms = { normalize_D: :nfd, normalize_C: :nfc, normalize_KD: :nfkd, normalize_KC: :nfkc }
    differences =
      forms.flat_map do |method, form|
        every_character.filter_map do |char|
          expected = assigned_in_unicode_6_2?(char) ? char.unicode_normalize(form) : char
          "#{method} #{codepoint_label(char)}" unless described_class.public_send(method, char) == expected
        end
      end

    expect(differences).to eq([])
  end

  it 'composes the canonical decomposition of every character back into NFC' do
    differences =
      every_character.reject do |char|
        decomposed = char.unicode_normalize(:nfd)
        described_class.compose(decomposed) == (assigned_in_unicode_6_2?(char) ? char.unicode_normalize(:nfc) : decomposed)
      end

    expect(differences.map { |char| codepoint_label(char) }).to eq([])
  end
end
