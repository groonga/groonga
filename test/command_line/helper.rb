require "fileutils"
require "json"
require "shellwords"
require "tempfile"

require "test-unit"

require_relative "helper/sandbox"
require_relative "helper/command_runner"

class GroongaTestCase < Test::Unit::TestCase
  include Sandbox
  include CommandRunner

  setup :setup_sandbox, :before => :prepend
  teardown :teardown_sandbox, :after => :append
end
