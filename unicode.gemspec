# frozen_string_literal: true

Gem::Specification.new do |s|
  s.name = 'unicode'
  s.version = '0.4.4.5'
  s.authors = ['Yoshida Masato', 'Ryan Kerr']
  s.email = %w[yoshidam@yoshidam.net ryan@magellan.ai]
  s.homepage = 'https://github.com/magellan-ai/unicode'
  s.license = 'Ruby'
  s.summary = 'Unicode normalization, case mapping, and character properties.'
  s.description = 'A C extension providing Unicode normalization (NFD, NFC, NFKD, NFKC), case mapping, general categories, text elements, and display width, built from Unicode 6.2 character data.'
  s.metadata = { 'rubygems_mfa_required' => 'true' }
  s.required_ruby_version = '>= 3.3'
  s.files = Dir['README*', 'LICENSE*', 'lib/**/*.rb', 'ext/**/*.{c,h,map,rb}']
  s.extensions = ['ext/unicode/extconf.rb']
  s.extra_rdoc_files = %w[README]
end
