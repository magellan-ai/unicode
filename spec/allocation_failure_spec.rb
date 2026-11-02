# frozen_string_literal: true

RSpec.describe Unicode, if: defined?(Unicode::FaultInjection) do
  it 'raises NoMemoryError from every failed buffer allocation, then works normally again' do
    unraised = []
    AllocationFailureSweep.run { |name, nth, error, _leaked| unraised << "#{name} allocation #{nth}" unless error }

    expect(unraised).to eq([])
    AllocationFailureSweep::CALLS.each_value { |call| expect { call.call }.not_to raise_error }
  end

  it 'frees every buffer when an allocation fails' do
    leaks = []
    AllocationFailureSweep.run { |name, nth, _error, leaked| leaks << "#{name} allocation #{nth}: #{leaked}" unless leaked.zero? }

    expect(leaks).to eq([])
  end
end
