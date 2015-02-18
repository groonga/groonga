module QueryOptimizer
  def setup_expression(domain)
    @expression = Groonga::Expression.new
    @expression.define_variable(:domain => domain)
  end

  def teardown_expression
    @expression.close
  end

  def parse(text, options={})
    default_options = {
      :syntax => :script,
    }
    @expression.parse(text, default_options.merge(options))
  end

  def dump_plan(text, options={})
    parse(text, options)
    @expression.dump_plan
  end
end
