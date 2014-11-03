module QueryOptimizer
  def create_expression(domain)
    expression = Groonga::Expression.new
    expression.define_variable(:domain => domain)
    expression
  end

  def dump_plan(expression, text, options={})
    default_options = {
      :syntax => :script,
    }
    expression.parse(text, default_options.merge(options))
    expression.dump_plan
  end
end
