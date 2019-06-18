require "fileutils"
require "json"
require "shellwords"
require "tempfile"

require "test-unit"

require_relative "helper/sandbox"
require_relative "helper/command_runner"
require_relative "helper/groonga_log"

class GroongaTestCase < Test::Unit::TestCase
  include Sandbox
  include CommandRunner
  include GroongaLog

  setup :setup_sandbox, :before => :prepend
  teardown :teardown_sandbox, :after => :append
end
