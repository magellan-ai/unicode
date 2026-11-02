# frozen_string_literal: true

RSpec.describe Unicode do
  it 'defines VERSION once, matching the version the gemspec ships' do
    gemspec_version, = run_ruby('-e', 'print Gem::Specification.load("unicode.gemspec").version')
    loaded_version, status = run_ruby('-w', '-Ilib', '-e', 'require "unicode"; print Unicode::VERSION')

    expect(status).to be_success
    expect(loaded_version).to eq(gemspec_version)
  end
end
