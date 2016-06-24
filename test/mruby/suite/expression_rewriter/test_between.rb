class TestBetween < ExpressionRewriterTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("expression_rewriters",
                          :type => :hash,
                          :key_type => :short_text) do |table|
        table.text("plugin_name")
      end

      schema.create_table("Logs") do |table|
        table.time("timestamp")
      end
    end

    @rewriters = Groonga["expression_rewriters"]
    @rewriters.add("optimizer",
                   :plugin_name => "expression_rewriters/optimizer")

    @logs = Groonga["Logs"]
    setup_expression(@logs)
  end

  def teardown
    teardown_expression
  end

  def test_neighbor
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

  def test_spread
    code =
      "timestamp <  '2015-11-00 00:00:00' && " +
      "1 == 1 && " +
      "true && " +
      "timestamp >= '2015-10-01 00:00:00'"
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
[1]
  op:         <equal>
  logical_op: <and>
  query:      <1>
  expr:       <7..9>
[2]
  op:         <push>
  logical_op: <and>
  query:      <(NULL)>
  expr:       <11..11>
    DUMP
  end

  def test_function
    code =
      "between(timestamp, '2015-09-01 00:00:00', 'include', '2015-10-15 00:00:00', 'include') && " +
      "timestamp >= '2015-10-01 00:00:00' && " +
      "timestamp <  '2015-11-00 00:00:00'"
    assert_equal(<<-DUMP, dump_rewritten_plan(code))
[0]
  op:         <call>
  logical_op: <or>
  args[0]:    <#<proc:function between arguments:[]>>
  args[1]:    <#<column:fix_size Logs.timestamp range:Time type:scalar compress:none>>
  args[2]:    <"2015-09-01 00:00:00">
  args[3]:    <"include">
  args[4]:    <"2015-10-15 00:00:00">
  args[5]:    <"include">
  expr:       <0..6>
[1]
  op:         <call>
  logical_op: <and>
  args[0]:    <#<proc:function between arguments:[]>>
  args[1]:    <#<column:fix_size Logs.timestamp range:Time type:scalar compress:none>>
  args[2]:    <"2015-10-01 00:00:00">
  args[3]:    <"include">
  args[4]:    <"2015-11-00 00:00:00">
  args[5]:    <"exclude">
  expr:       <7..13>
    DUMP
  end
end
