# frozen_string_literal: true

RSpec.describe Unicode do
  describe '.width' do
    it 'counts narrow characters as one column and wide characters as two' do
      expect(described_class.width('abc')).to eq(3)
      expect(described_class.width('あいう')).to eq(6)
      expect(described_class.width("\uFF76\uFF9E")).to eq(2)
      expect(described_class.width('')).to eq(0)
    end

    it 'counts combining marks as zero columns' do
      expect(described_class.width("e\u0301")).to eq(1)
    end

    it 'counts East Asian ambiguous characters as wide only when asked to' do
      expect(described_class.width('α')).to eq(1)
      expect(described_class.width('α', true)).to eq(2)
    end

    it 'returns -1 for a string containing a control character' do
      expect(described_class.width("a\tb")).to eq(-1)
    end
  end
end
