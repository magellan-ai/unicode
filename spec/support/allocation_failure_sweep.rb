# frozen_string_literal: true

# Needs the fault-injection build that `rake spec:sanitized` compiles.
module AllocationFailureSweep
  LONG_TEXT = ([0x1E08, 0x00C5, 0xFB03, 0xAC00, 0x41, 0x301].pack('U*') * 400).freeze
  OTHER_LONG_TEXT = ([0x1E08, 0x00C5, 0xFB03, 0xAC00, 0x42, 0x301].pack('U*') * 400).freeze

  CALLS = {
    strcmp:                     -> { Unicode.strcmp(LONG_TEXT, OTHER_LONG_TEXT) },
    strcmp_compat:              -> { Unicode.strcmp_compat(LONG_TEXT, OTHER_LONG_TEXT) },
    decompose:                  -> { Unicode.decompose(LONG_TEXT) },
    decompose_safe:             -> { Unicode.decompose_safe(LONG_TEXT) },
    decompose_compat:           -> { Unicode.decompose_compat(LONG_TEXT) },
    compose:                    -> { Unicode.compose(LONG_TEXT) },
    normalize_C:                -> { Unicode.normalize_C(LONG_TEXT) },
    normalize_C_safe:           -> { Unicode.normalize_C_safe(LONG_TEXT) },
    normalize_KC:               -> { Unicode.normalize_KC(LONG_TEXT) },
    upcase:                     -> { Unicode.upcase(LONG_TEXT) },
    downcase:                   -> { Unicode.downcase(LONG_TEXT) },
    capitalize:                 -> { Unicode.capitalize(LONG_TEXT) },
    categories:                 -> { Unicode.categories(LONG_TEXT) },
    abbr_categories_with_block: -> { Unicode.abbr_categories(LONG_TEXT) { |category| category } },
    width:                      -> { Unicode.width(LONG_TEXT, true) },
    text_elements:              -> { Unicode.text_elements(LONG_TEXT) },
    text_elements_with_block:   -> { Unicode.text_elements(LONG_TEXT) { |element| element } }
  }.freeze

  module_function

  def run
    CALLS.each do |name, call|
      (1..allocations_made_by(call)).each do |nth|
        live_buffers = Unicode::FaultInjection.live_buffers
        error = raised_by(call, failing: nth)
        yield name, nth, error, Unicode::FaultInjection.live_buffers - live_buffers
      end
    end
  end

  def allocations_made_by(call)
    before = Unicode::FaultInjection.allocations
    call.call
    Unicode::FaultInjection.allocations - before
  end

  def raised_by(call, failing:)
    Unicode::FaultInjection.fail_allocation(failing)
    call.call
    nil
  rescue NoMemoryError => e
    e
  ensure
    Unicode::FaultInjection.fail_allocation(0)
  end
end
