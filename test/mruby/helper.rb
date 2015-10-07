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
  def dump_plan(text, options={})
    parse(text, options)
    @expression.dump_plan
  end
end

class ExpressionRewriterTestCase < GroongaTestCase
  def dump_rewritten_plan(text, options={})
    parse(text, options)
    rewritten_expression = @expression.rewrite
    if rewritten_expression
      rewritten_expression.dump_plan
    else
      nil
    end
  end
end
