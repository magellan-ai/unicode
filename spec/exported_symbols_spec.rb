# frozen_string_literal: true

RSpec.describe Unicode do
  it 'exports nothing from the compiled extension but its Init function' do
    binary = $LOADED_FEATURES.grep(/unicode_native\.(?:bundle|so)\z/).first
    listing =
      begin
        RUBY_PLATFORM.include?('darwin') ? `nm -gU #{binary}` : `nm -D --defined-only #{binary}`
      rescue Errno::ENOENT
        skip 'nm is not available'
      end
    exported = listing.lines.map { |line| line.split.last.delete_prefix('_') }

    expect(Process.last_status).to be_success
    expect(exported).to eq(['Init_unicode_native'])
  end
end
