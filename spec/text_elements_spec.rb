# frozen_string_literal: true

RSpec.describe Unicode do
  describe '.text_elements' do
    it 'groups base characters with their combining marks and Hangul jamo into syllables' do
      expect(described_class.text_elements("e\u0301a\u1100\u1161\u11A8\uAC00\u11A8\u0301x")).to eq(
        ["e\u0301", 'a', "\u1100\u1161\u11A8", "\uAC00\u11A8\u0301", 'x']
      )
    end

    it 'yields each element to a block and returns the string' do
      yielded = []

      expect(described_class.text_elements("ae\u0301") { |element| yielded << element }).to eq("ae\u0301")
      expect(yielded).to eq(['a', "e\u0301"])
    end
  end
end
