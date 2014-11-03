require "fileutils"

require "test-unit"

require_relative "helper/sandbox"
require_relative "helper/query_optimizer"

class GroongaTestCase < Test::Unit::TestCase
  include Sandbox

  setup :setup_sandbox, :before => :prepend
  teardown :teardown_sandbox, :after => :append
end

class QueryOptimizerTestCase < GroongaTestCase
  include QueryOptimizer
end
