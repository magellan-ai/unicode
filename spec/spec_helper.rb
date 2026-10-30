# frozen_string_literal: true

require 'simplecov'

SimpleCov.start do
  enable_coverage :branch

  skip '/spec/'
end

$LOAD_PATH.unshift(File.expand_path('../lib', __dir__))

require 'unicode'

RSpec.configure do |config|
  config.disable_monkey_patching!

  config.expect_with :rspec do |expectations|
    expectations.syntax = :expect
  end
end
