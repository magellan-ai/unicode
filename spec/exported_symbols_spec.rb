# frozen_string_literal: true

RSpec.describe Unicode do
  it 'exports nothing from the compiled extension but its Init function' do
    binary = $LOADED_FEATURES.grep(/unicode_native\.(?:bundle|so)\z/).first
    listing = RUBY_PLATFORM.include?('darwin') ? `nm -gU #{binary}` : `nm -D --defined-only #{binary}`
    skip 'nm is not available' unless Process.last_status.success?

    exported = listing.lines.map { |line| line.split.last.delete_prefix('_') }

    expect(exported).to eq(['Init_unicode_native'])
  end
end
