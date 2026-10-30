# frozen_string_literal: true

require 'rbconfig'

module SubprocessHelpers
  PROJECT_ROOT = File.expand_path('../..', __dir__)

  # Open3 threads crash under ASan for `rake spec:sanitized`, so use IO.popen instead.
  def run_ruby(*arguments)
    output = IO.popen([RbConfig.ruby, *arguments], chdir: PROJECT_ROOT, err: %i[child out], &:read)

    [output, Process.last_status]
  end
end
