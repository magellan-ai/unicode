# frozen_string_literal: true

RSpec.describe Unicode do
  let(:mixed) { "Aa1 \u0301\u3042" }

  describe '.categories' do
    it 'returns the long general category name of each character' do
      expect(described_class.categories(mixed)).to eq(
        %i[Uppercase_Letter Lowercase_Letter Decimal_Number Space_Separator Nonspacing_Mark Other_Letter]
      )
    end

    it 'yields each name to a block and returns the string' do
      yielded = []

      expect(described_class.categories('Ab') { |name| yielded << name }).to eq('Ab')
      expect(yielded).to eq(%i[Uppercase_Letter Lowercase_Letter])
    end
  end

  describe '.abbr_categories' do
    it 'returns the abbreviated general category name of each character' do
      expect(described_class.abbr_categories(mixed)).to eq(%i[Lu Ll Nd Zs Mn Lo])
    end

    it 'resolves ranges, private use, and characters unassigned in Unicode 6.2' do
      expect(described_class.abbr_categories("\u4E00\uAC00\uE000\u0378\u{1F600}")).to eq(%i[Lo Lo Co Cn So])
    end

    it 'yields each name to a block and returns the string' do
      yielded = []

      expect(described_class.abbr_categories('Ab') { |name| yielded << name }).to eq('Ab')
      expect(yielded).to eq(%i[Lu Ll])
    end
  end
end
