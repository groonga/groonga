require "fileutils"
require "json"
require "shellwords"
require "tempfile"
require "time"

require "test-unit"

require_relative "helper/sandbox"
require_relative "helper/command_runner"
require_relative "helper/groonga_log"
require_relative "helper/platform"
require_relative "helper/fixture"

class GroongaTestCase < Test::Unit::TestCase
  include ::Sandbox
  include ::CommandRunner
  include ::GroongaLog
  include ::Platform
  include ::Fixture

  setup :setup_sandbox, :before => :prepend
  teardown :teardown_sandbox, :after => :append
end
