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

  it 'reserves the whole decode buffer before reading the argument, so decoding never allocates' do
    decode_only_calls = AllocationFailureSweep::CALLS.slice(:width, :categories)

    expect(decode_only_calls.transform_values { |call| AllocationFailureSweep.allocations_made_by(call) })
      .to eq(width: 1, categories: 1)
  end
end
