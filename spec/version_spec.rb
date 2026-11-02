# frozen_string_literal: true

require 'open3'
require 'rbconfig'

RSpec.describe Unicode do
  let(:project_root) { File.expand_path('..', __dir__) }

  it 'defines VERSION once, matching the version the gemspec ships' do
    gemspec_version, = Open3.capture3(RbConfig.ruby, '-e', 'print Gem::Specification.load("unicode.gemspec").version', chdir: project_root)
    loaded_version, warnings, status = Open3.capture3(RbConfig.ruby, '-w', '-Ilib', '-e', 'require "unicode"; print Unicode::VERSION', chdir: project_root)

    expect([status.success?, warnings, loaded_version]).to eq([true, '', gemspec_version])
  end
end
