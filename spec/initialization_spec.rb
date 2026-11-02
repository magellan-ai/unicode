# frozen_string_literal: true

RSpec.describe Unicode do
  it 'loads normally when require is retried after the extension failed to initialize' do
    script = <<~RUBY
      module Unicode
        @failures_left = 1

        def self.singleton_method_added(name)
          return if name == :singleton_method_added || !@failures_left.positive?

          @failures_left -= 1
          raise "failed while defining \#{name}"
        end
      end

      begin
        require 'unicode'
      rescue RuntimeError => e
        puts e.message
      end
      require 'unicode'
      print Unicode.nfc([0x65, 0x301].pack('U*')).codepoints.inspect
    RUBY

    output, status = run_ruby('-Ilib', '-e', script)

    expect(output).to eq("failed while defining strcmp\n[233]")
    expect(status).to be_success
  end
end
