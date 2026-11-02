# frozen_string_literal: true

RSpec.describe Unicode do
  describe '.upcase' do
    it 'upcases, including fullwidth letters and decomposed sequences' do
      expect(described_class.upcase("c\u0301\u0327\uFF41")).to eq("C\u0301\u0327\uFF21")
    end

    it 'applies multi-character special casings' do
      expect(described_class.upcase('ß')).to eq('SS')
      expect(described_class.upcase("\uFB03")).to eq('FFI')
    end

    it 'maps digraphs to their uppercase form' do
      expect(described_class.upcase("\u01C6")).to eq("\u01C4")
    end
  end

  describe '.downcase' do
    it 'downcases, including fullwidth letters and decomposed sequences' do
      expect(described_class.downcase('HELLO World')).to eq('hello world')
      expect(described_class.downcase("C\u0301\u0327\uFF21")).to eq("c\u0301\u0327\uFF41")
    end

    it 'applies the unconditional special casings only' do
      expect(described_class.downcase("\u0130")).to eq("i\u0307")
      expect(described_class.downcase('ΣΑΣ')).to eq('σασ')
    end

    it 'returns an empty string unchanged' do
      expect(described_class.downcase('')).to eq('')
    end
  end

  describe '.capitalize' do
    it 'titlecases the first character and downcases the rest' do
      expect(described_class.capitalize('HELLO')).to eq('Hello')
      expect(described_class.capitalize("\u01F1Aa\uFF21")).to eq("\u01F2aa\uFF41")
      expect(described_class.capitalize("\u01C6emal")).to eq("\u01C5emal")
    end
  end
end
