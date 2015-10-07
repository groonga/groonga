class TestBetween < ExpressionRewriterTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("rewriters",
                          :type => :hash,
                          :key_type => :short_text) do |table|
        table.text("plugin_name")
      end

      schema.create_table("Logs") do |table|
        table.time("timestamp")
      end
    end

    @rewriters = Groonga["rewriters"]
    @rewriters.add("optimize", :plugin_name => "expression_rewriters/optimize")

    @logs = Groonga["Logs"]
    setup_expression(@logs)
  end

  def teardown
    teardown_expression
  end

  def test_optimizable
    code =
      "timestamp >= '2015-10-01 00:00:00' && " +
      "timestamp <  '2015-11-00 00:00:00'"
    assert_equal(<<-DUMP, dump_rewritten_plan(code))
[0]
  op:         <call>
  logical_op: <or>
  args[0]:    <#<proc:function between arguments:[]>>
  args[1]:    <#<column:fix_size Logs.timestamp range:Time type:scalar compress:none>>
  args[2]:    <"2015-10-01 00:00:00">
  args[3]:    <"include">
  args[4]:    <"2015-11-00 00:00:00">
  args[5]:    <"exclude">
  expr:       <0..6>
    DUMP
  end
end
