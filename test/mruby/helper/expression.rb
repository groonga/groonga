module Expression
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
end
