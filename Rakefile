# frozen_string_literal: true

require 'bundler/gem_tasks'
require 'rake/clean'
require 'rake/extensiontask'
require 'rspec/core/rake_task'
require 'rubocop/rake_task'

CLEAN.include('pkg')

Rake::ExtensionTask.new('unicode_native') do |ext|
  ext.ext_dir = 'ext/unicode'
  ext.lib_dir = 'lib/unicode'
end

RSpec::Core::RakeTask.new(:spec)

namespace :spec do
  desc 'Run the specs against an AddressSanitizer and UndefinedBehaviorSanitizer build (Linux with gcc)'
  task :sanitized do
    asan_runtime = `gcc -print-file-name=libasan.so`.strip
    abort 'spec:sanitized needs gcc and its libasan.so runtime (Linux); see CLAUDE.md for running it in Docker.' unless File.exist?(asan_runtime)

    ENV['UNICODE_SANITIZE'] = '1'
    Rake::Task[:clobber].invoke
    Rake::Task[:compile].invoke
    sh({ 'LD_PRELOAD' => asan_runtime, 'ASAN_OPTIONS' => 'detect_leaks=0' }, RbConfig.ruby, '-S', 'rspec')
  ensure
    Rake::Task[:clobber].execute
  end
end

RuboCop::RakeTask.new

task default: %i[compile spec]
