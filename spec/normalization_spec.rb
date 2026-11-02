# frozen_string_literal: true

RSpec.describe Unicode do
  let(:circled_ten_office) { "\u247D o\uFB03ce" }
  let(:angstrom_sign) { "\u212B" }
  let(:devanagari_qa) { "\u0958" }
  let(:halfwidth_and_fullwidth_ga) { "\uFF76\uFF9E\u30AC" }

  describe '.decompose' do
    it 'canonically decomposes, leaving compatibility characters alone' do
      expect(described_class.decompose(circled_ten_office)).to eq(circled_ten_office)
      expect(described_class.decompose(angstrom_sign)).to eq("A\u030A")
      expect(described_class.decompose(devanagari_qa)).to eq("\u0915\u093C")
    end

    it 'sorts combining marks into canonical order' do
      expect(described_class.decompose("c\u0301\u0327")).to eq("c\u0327\u0301")
      expect(described_class.decompose("\u0107\u0327")).to eq("c\u0327\u0301")
      expect(described_class.decompose("\u00E7\u0301")).to eq("c\u0327\u0301")
    end

    it 'decomposes Hangul syllables algorithmically' do
      expect(described_class.decompose("\uC694\uC2DC\uB2F4"))
        .to eq("\u110B\u116D\u1109\u1175\u1103\u1161\u11B7")
    end

    it 'is aliased as .normalize_D and .nfd' do
      expect(described_class.normalize_D(halfwidth_and_fullwidth_ga)).to eq("\uFF76\uFF9E\u30AB\u3099")
      expect(described_class.nfd(halfwidth_and_fullwidth_ga)).to eq("\uFF76\uFF9E\u30AB\u3099")
    end
  end

  describe '.decompose_safe' do
    it 'decomposes like .decompose, except for composition exclusions' do
      expect(described_class.decompose_safe("\u00C5")).to eq("A\u030A")
      expect(described_class.decompose_safe(angstrom_sign)).to eq(angstrom_sign)
      expect(described_class.decompose_safe(devanagari_qa)).to eq(devanagari_qa)
    end

    it 'is aliased as .normalize_D_safe and .nfd_safe' do
      expect(described_class.normalize_D_safe(devanagari_qa)).to eq(devanagari_qa)
      expect(described_class.nfd_safe(devanagari_qa)).to eq(devanagari_qa)
    end
  end

  describe '.decompose_compat' do
    it 'decomposes compatibility characters too' do
      expect(described_class.decompose_compat(circled_ten_office)).to eq('(10) office')
    end

    it 'is aliased as .normalize_KD and .nfkd' do
      expect(described_class.normalize_KD(halfwidth_and_fullwidth_ga)).to eq("\u30AB\u3099\u30AB\u3099")
      expect(described_class.nfkd(halfwidth_and_fullwidth_ga)).to eq("\u30AB\u3099\u30AB\u3099")
    end
  end

  describe '.compose' do
    it 'composes decomposed input, sorting combining marks first' do
      expect(described_class.compose("e\u0301")).to eq("\u00E9")
      expect(described_class.compose("c\u0301\u0327")).to eq("\u1E09")
    end

    it 'composes Hangul jamo algorithmically' do
      expect(described_class.compose("\u110B\u116D\u1109\u1175\u1103\u1161\u11B7"))
        .to eq("\uC694\uC2DC\uB2F4")
    end
  end

  describe '.normalize_C' do
    it 'decomposes and then recomposes, including composition exclusions' do
      expect(described_class.normalize_C("\u0107\u0327")).to eq("\u1E09")
      expect(described_class.normalize_C(angstrom_sign)).to eq("\u00C5")
      expect(described_class.normalize_C(devanagari_qa)).to eq("\u0915\u093C")
    end

    it 'is aliased as .nfc' do
      expect(described_class.nfc(halfwidth_and_fullwidth_ga)).to eq(halfwidth_and_fullwidth_ga)
    end
  end

  describe '.normalize_C_safe' do
    it 'recomposes without decomposing composition exclusions' do
      expect(described_class.normalize_C_safe(angstrom_sign)).to eq(angstrom_sign)
      expect(described_class.normalize_C_safe(devanagari_qa)).to eq(devanagari_qa)
      expect(described_class.normalize_C_safe("\u0107\u0327")).to eq("\u1E09")
    end

    it 'is aliased as .nfc_safe' do
      expect(described_class.nfc_safe(angstrom_sign)).to eq(angstrom_sign)
    end
  end

  describe '.normalize_KC' do
    it 'decomposes compatibility characters and then recomposes' do
      expect(described_class.normalize_KC(halfwidth_and_fullwidth_ga)).to eq("\u30AC\u30AC")
      expect(described_class.normalize_KC(circled_ten_office)).to eq('(10) office')
    end

    it 'is aliased as .nfkc' do
      expect(described_class.nfkc(halfwidth_and_fullwidth_ga)).to eq("\u30AC\u30AC")
    end
  end
end
