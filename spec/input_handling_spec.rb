# frozen_string_literal: true

RSpec.describe Unicode do
  let(:string_returning_methods) do
    %i[
      decompose decompose_safe decompose_compat compose
      normalize_C normalize_C_safe normalize_KC
      upcase downcase capitalize
    ]
  end
  let(:string_reading_methods) { [*string_returning_methods, :categories, :abbr_categories, :text_elements, :width] }

  it 'raises TypeError for a non-String argument' do
    string_reading_methods.each do |method|
      expect { described_class.public_send(method, :abc) }.to raise_error(TypeError)
    end
    expect { described_class.strcmp('abc', nil) }.to raise_error(TypeError)
  end

  it 'transcodes non-UTF-8 input and returns UTF-8' do
    iso_8859_1_input = "\u00C0b".encode('ISO-8859-1')

    expect(described_class.downcase(iso_8859_1_input)).to eq("\u00E0b")
    string_returning_methods.each do |method|
      expect(described_class.public_send(method, iso_8859_1_input).encoding).to eq(Encoding::UTF_8)
    end
  end

  it 'accepts binary input that is plain ASCII' do
    expect(described_class.downcase('AB'.b)).to eq('ab')
  end

  it 'raises when binary input has bytes that cannot become UTF-8' do
    expect { described_class.downcase("A\xFF".b) }.to raise_error(Encoding::UndefinedConversionError)
  end

  it 'returns a new string rather than the argument' do
    argument = 'abc'

    expect(described_class.downcase(argument)).not_to equal(argument)
  end

  it 'maps an empty string to an empty string' do
    string_returning_methods.each do |method|
      expect(described_class.public_send(method, '')).to eq('')
    end
    expect([described_class.categories(''), described_class.text_elements(''), described_class.width('')]).to eq([[], [], 0])
  end

  it 'raises ArgumentError for bytes that are not valid UTF-8, like String#unicode_normalize' do
    invalid_bytes_message = 'invalid byte sequence in UTF-8'
    malformed_inputs = {
      impossible_byte:     "AB\xFFCD",
      truncated_character: "AB\xE3\x81",
      overlong_slash:      "AB\xC0\xAFCD",
      surrogate:           "AB\xED\xA0\x80CD",
      past_u10ffff:        "AB\xF4\x90\x80\x80CD"
    }

    aggregate_failures do
      malformed_inputs.each_value do |input|
        string_reading_methods.each do |method|
          expect { described_class.public_send(method, input) }.to raise_error(ArgumentError, invalid_bytes_message)
        end
        %i[strcmp strcmp_compat].each do |method|
          expect { described_class.public_send(method, input, 'AB') }.to raise_error(ArgumentError, invalid_bytes_message)
          expect { described_class.public_send(method, 'AB', input) }.to raise_error(ArgumentError, invalid_bytes_message)
        end
      end
      expect { described_class.downcase("A\xFF".dup.force_encoding('US-ASCII')) }.to raise_error(ArgumentError, invalid_bytes_message)
    end
  end

  it 'reads a US-ASCII-tagged string as UTF-8 bytes' do
    mislabeled_capital_e_acute = [0xC3, 0x89].pack('C*').force_encoding('US-ASCII')

    expect(described_class.downcase(mislabeled_capital_e_acute).codepoints).to eq([0xE9])
  end
end
