# frozen_string_literal: true

unless ENV.key?('UNICODE_SANITIZE')
  require 'simplecov'

  SimpleCov.start do
    enable_coverage :branch

    skip '/spec/'
  end
end

$LOAD_PATH.unshift(File.expand_path('../lib', __dir__))

require 'unicode'

Dir[File.join(__dir__, 'support', '**', '*.rb')].each { |support_file| require support_file }

RSpec.configure do |config|
  config.disable_monkey_patching!

  config.include CodepointHelpers
  config.include SubprocessHelpers

  config.expect_with :rspec do |expectations|
    expectations.syntax = :expect
  end
end
