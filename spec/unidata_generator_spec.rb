# frozen_string_literal: true

require 'rbconfig'
require 'tmpdir'
require 'zlib'

RSpec.describe 'tools/mkunidata.rb' do
  let(:project_root) { File.expand_path('..', __dir__) }

  it 'regenerates the committed unidata.map from the Unicode 6.2 data files' do
    generated =
      Dir.mktmpdir do |directory|
        sources =
          %w[UnicodeData DerivedNormalizationProps SpecialCasing EastAsianWidth].map do |name|
            File.join(directory, "#{name}.txt").tap do |source|
              File.binwrite(source, Zlib::GzipReader.open(File.join(__dir__, "fixtures/ucd-6.2.0/#{name}.txt.gz"), &:read))
            end
          end
        IO.popen([RbConfig.ruby, 'tools/mkunidata.rb', *sources], chdir: project_root, &:read)
      end
    committed = File.binread(File.join(project_root, 'ext/unicode/unidata.map'))

    expect(Process.last_status).to be_success
    first_differences = generated.b.lines.zip(committed.lines).each_with_index.reject { |(ours, theirs), _| ours == theirs }.first(3)
    expect([generated.lines.size, first_differences]).to eq([committed.lines.size, []])
  end
end
