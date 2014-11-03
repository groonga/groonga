class TestNoIndex < QueryOptimizerTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("Logs") do |table|
        table.text("message")
      end
    end

    @logs = Groonga["Logs"]
    @expression = create_expression(@logs)
  end

  def teardown
    @expression.close
  end

  def test_plus
    assert_equal(<<-DUMP, dump_plan(@expression, "1 + 1"))
sequential search
    DUMP
  end
end
