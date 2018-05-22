class TestInValues < ExpressionRewriterTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("expression_rewriters",
                          :type => :hash,
                          :key_type => :short_text) do |table|
        table.text("plugin_name")
      end

      schema.create_table("Events") do |table|
        table.time("timestamp")
        table.short_text("tag")
      end
    end

    @rewriters = Groonga["expression_rewriters"]
    @rewriters.add("optimizer",
                   :plugin_name => "expression_rewriters/optimizer")

    @events = Groonga["Events"]
    setup_expression(@events)
  end

  def teardown
    teardown_expression
  end

  def test_only
    code =
      "tag == 'run' || tag == 'crash' || tag == 'shutdown'"
    assert_equal(<<-DUMP, dump_rewritten_plan(code))
[0]
  op:         <call>
  logical_op: <or>
  args[0]:    <#<proc:function in_values arguments:[]>>
  args[1]:    <#<column:var_size Events.tag range:ShortText type:scalar compress:none>>
  args[2]:    <"run">
  args[3]:    <"crash">
  args[4]:    <"shutdown">
  expr:       <0..5>
    DUMP
  end

  def test_combined
    code =
      "tag != 'error' && (tag == 'run' || tag == 'crash' || tag == 'shutdown')"
    assert_equal(<<-DUMP, dump_rewritten_plan(code))
[0]
  op:         <not_equal>
  logical_op: <or>
  index:      <[]>
  query:      <"error">
  expr:       <0..2>
[1]
  op:         <call>
  logical_op: <and>
  args[0]:    <#<proc:function in_values arguments:[]>>
  args[1]:    <#<column:var_size Events.tag range:ShortText type:scalar compress:none>>
  args[2]:    <"run">
  args[3]:    <"crash">
  args[4]:    <"shutdown">
  expr:       <3..8>
    DUMP
  end
end
