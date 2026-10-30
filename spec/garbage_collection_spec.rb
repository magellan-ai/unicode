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
end
