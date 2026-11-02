# frozen_string_literal: true

require 'mkmf'

if ENV.key?('UNICODE_SANITIZE')
  sanitizers = '-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer'
  $CFLAGS << " #{sanitizers} -O1 -g -DUNICODE_FAULT_INJECTION"
  $LDFLAGS << " #{sanitizers}"
end

create_makefile('unicode/unicode_native')
