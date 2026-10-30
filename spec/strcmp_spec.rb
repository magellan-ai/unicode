# frozen_string_literal: true

RSpec.describe Unicode do
  let(:halfwidth_ga) { "\uFF76\uFF9E" }
  let(:fullwidth_ga) { "\u30AC" }

  describe '.strcmp' do
    it 'treats canonically equivalent strings as equal' do
      expect(described_class.strcmp("\u00E9", "e\u0301")).to eq(0)
    end

    it 'orders other strings by their canonical decompositions' do
      expect(described_class.strcmp('a', 'b')).to be_negative
      expect(described_class.strcmp(halfwidth_ga, fullwidth_ga)).to be_positive
      expect(described_class.strcmp(fullwidth_ga, halfwidth_ga)).to be_negative
    end

    it 'compares past an embedded NUL' do
      pending 'the comparison is a C strcmp, which stops at the first NUL'

      expect(described_class.strcmp("a\0b", "a\0c")).to be_negative
    end
  end

  describe '.strcmp_compat' do
    it 'treats compatibility equivalent strings as equal' do
      expect(described_class.strcmp_compat(halfwidth_ga, fullwidth_ga)).to eq(0)
    end
  end
end
