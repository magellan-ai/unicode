# frozen_string_literal: true

RSpec.describe Unicode do
  it 'looks characters up without going through Ruby hashes' do
    script = <<~RUBY
      require 'unicode'

      class Hash
        def default(*)
          raise 'looked up through a Ruby Hash'
        end
      end

      unassigned_then_capital_a = [0x378, 0x41].pack('U*')
      e_then_combining_acute = [0x65, 0x301].pack('U*')
      p [Unicode.downcase(unassigned_then_capital_a).codepoints, Unicode.nfc(e_then_combining_acute).codepoints]
    RUBY

    output, status = run_ruby('-Ilib', '-e', script)

    expect(output).to eq("[[888, 97], [233]]\n")
    expect(status).to be_success
  end
end
