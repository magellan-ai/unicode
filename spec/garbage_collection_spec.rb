# frozen_string_literal: true

RSpec.describe Unicode do
  # Run in a fresh process to test the load itself
  it 'keeps working after loading under GC stress and a full compaction' do
    script = <<~RUBY
      GC.stress = true
      require 'unicode'
      GC.stress = false
      GC.verify_compaction_references(expand_heap: true, toward: :empty)
      print Array.new(1_000) { Unicode.downcase("\\u00C0BC") }.uniq.inspect
    RUBY

    output, status = run_ruby('-Ilib', '-e', script)

    expect(status).to be_success
    expect(output).to eq(["\u00E0bc"].inspect)
  end

  it 'keeps a transcoded argument alive while its buffers allocate under GC stress' do
    script = <<~RUBY
      require 'unicode'

      iso_8859_1_text = ([0xC0, 0x62, 0xE9, 0x20].pack('C*') * 300).force_encoding('ISO-8859-1')
      calls = [
        ->(text) { Unicode.downcase(text) },
        ->(text) { Unicode.nfc(text) },
        ->(text) { Unicode.strcmp(text, text) },
        ->(text) { Unicode.categories(text) },
        ->(text) { Unicode.width(text) },
        ->(text) { Unicode.text_elements(text) }
      ]
      expected = calls.map { |call| call.call(iso_8859_1_text.dup) }
      GC.stress = true
      stressed = calls.map { |call| call.call(iso_8859_1_text.dup) }
      GC.stress = false
      print stressed == expected
    RUBY

    output, status = run_ruby('-Ilib', '-e', script)

    expect(output).to eq('true')
    expect(status).to be_success
  end
end
