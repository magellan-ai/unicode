# frozen_string_literal: true

require 'zlib'

RSpec.describe Unicode do
  let(:normalization_tests) do
    Zlib::GzipReader.open(File.expand_path('fixtures/ucd-6.2.0/NormalizationTest.txt.gz', __dir__)) do |gz|
      gz.each_line.reject { |line| line.start_with?('#', '@') }.map do |line|
        line.split(';').first(5).map { |column| column.split.map(&:hex).pack('U*') }
      end
    end
  end

  it 'reads every test line of the Unicode 6.2 NormalizationTest.txt' do
    expect(normalization_tests.size).to eq(18_377)
  end

  it 'conforms to the Unicode 6.2 normalization test suite' do
    failures =
      normalization_tests.reject do |source, nfc, nfd, nfkc, nfkd|
        [source, nfc, nfd].all? { |string| described_class.normalize_D(string) == nfd } &&
          [nfkc, nfkd].all? { |string| described_class.normalize_D(string) == nfkd } &&
          [source, nfc, nfd].all? { |string| described_class.normalize_C(string) == nfc } &&
          [nfkc, nfkd].all? { |string| described_class.normalize_C(string) == nfkc } &&
          [source, nfc, nfd, nfkc, nfkd].all? { |string| described_class.normalize_KD(string) == nfkd } &&
          [source, nfc, nfd, nfkc, nfkd].all? { |string| described_class.normalize_KC(string) == nfkc }
      end

    expect(failures.map { |columns| codepoint_label(columns.first) }).to eq([])
  end
end
