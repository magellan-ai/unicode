# frozen_string_literal: true

RSpec.describe Unicode do
  let(:halfwidth_ga) { "\uFF76\uFF9E" }
  let(:fullwidth_ga) { "\u30AC" }
  let(:samples) do
    [
      [], [0x61], [0x62], [0x61, 0x62], [0x61, 0x62, 0x63], [0x61, 0x00, 0x62], [0x7F], [0x80],
      [0xE9], [0x65, 0x301], [0xDF], [0x30AC], [0xFF76, 0xFF9E], [0xAC00], [0x1D400], [0x1F600]
    ].map { |codepoints| codepoints.pack('U*') }
  end

  describe '.strcmp' do
    it 'treats canonically equivalent strings as equal' do
      expect(described_class.strcmp("\u00E9", "e\u0301")).to eq(0)
    end

    it 'orders other strings by their canonical decompositions' do
      expect(described_class.strcmp('a', 'b')).to be_negative
      expect(described_class.strcmp(halfwidth_ga, fullwidth_ga)).to be_positive
      expect(described_class.strcmp(fullwidth_ga, halfwidth_ga)).to be_negative
    end

    it 'compares past an embedded NUL, and orders a prefix first' do
      expect(described_class.strcmp("a\0b", "a\0c")).to eq(-1)
      expect(described_class.strcmp("a\0c", "a\0b")).to eq(1)
      expect(described_class.strcmp('ab', 'abc')).to eq(-1)
    end

    it 'returns -1, 0, or 1, ordering by the bytes of the canonical decompositions' do
      samples.product(samples).each do |left, right|
        expect(described_class.strcmp(left, right)).to eq(described_class.nfd(left).b <=> described_class.nfd(right).b)
      end
    end
  end

  describe '.strcmp_compat' do
    it 'treats compatibility equivalent strings as equal' do
      expect(described_class.strcmp_compat(halfwidth_ga, fullwidth_ga)).to eq(0)
    end

    it 'compares past an embedded NUL' do
      expect(described_class.strcmp_compat("a\0b", "a\0c")).to eq(-1)
    end

    it 'returns -1, 0, or 1, ordering by the bytes of the compatibility decompositions' do
      samples.product(samples).each do |left, right|
        expect(described_class.strcmp_compat(left, right)).to eq(described_class.nfkd(left).b <=> described_class.nfkd(right).b)
      end
    end
  end
end
