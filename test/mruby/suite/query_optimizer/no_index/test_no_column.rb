class TestNoIndexNoColumn < QueryOptimizerTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("Logs") do |table|
        table.text("message")
      end
    end

    @logs = Groonga["Logs"]
    setup_expression(@logs)
  end

  def teardown
    teardown_expression
  end

  def test_plus
    assert_equal(<<-DUMP, dump_plan("1 + 1"))
sequential search
    DUMP
  end
end
