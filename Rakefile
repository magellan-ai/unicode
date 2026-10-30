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

RuboCop::RakeTask.new

task default: %i[compile spec]
