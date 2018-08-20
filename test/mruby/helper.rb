require "fileutils"

require "test-unit"

require_relative "helper/sandbox"
require_relative "helper/expression"

class GroongaTestCase < Test::Unit::TestCase
  include Sandbox
  include Expression

  setup :setup_sandbox, :before => :prepend
  teardown :teardown_sandbox, :after => :append
end

class QueryOptimizerTestCase < GroongaTestCase
  setup do |&test|
    change_env("GRN_EXPR_OPTIMIZE", "yes") do
      test.call
    end
  end

  def change_env(key, value)
    current_value = ENV[key]
    begin
      ENV[key] = value
      yield
    ensure
      ENV[key] = current_value
    end
  end

  def dump_plan(text, options={})
    parse(text, options)
    @expression.dump_plan
  end
end
