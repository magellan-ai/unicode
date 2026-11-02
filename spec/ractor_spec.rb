# frozen_string_literal: true

RSpec.describe Unicode, unless: ENV.key?('UNICODE_SANITIZE') do
  it 'works from several Ractors at once, giving the same results as the main Ractor' do
    script = <<~RUBY
      Warning[:experimental] = false
      require 'unicode'

      exercise = nil.instance_exec do
        lambda do
          text = [0x1E08, 0x00C5, 0xFB03, 0xAC00, 0x41, 0x301].pack('U*') * 50
          [
            Unicode.downcase(text),
            Unicode.nfc(text),
            Unicode.nfkd(text),
            Unicode.strcmp(text, text),
            Unicode.categories(text),
            Unicode.width(text),
            Unicode.text_elements(text)
          ]
        end
      end
      expected = exercise.call
      ractors = Array.new(4) { Ractor.new(&Ractor.make_shareable(exercise)) }
      results = ractors.map { |ractor| ractor.respond_to?(:value) ? ractor.value : ractor.take }
      print results.all?(expected)
    RUBY

    output, status = run_ruby('-Ilib', '-e', script)

    expect(output).to eq('true')
    expect(status).to be_success
  end
end
