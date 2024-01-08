class TestInValues < QueryOptimizerTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("Events") do |table|
        table.time("timestamp")
        table.short_text("tag")
      end
    end

    @events = Groonga["Events"]
    setup_expression(@events)
  end

  def teardown
    teardown_expression
  end

  def test_only
    code =
      "tag == 'run' || tag == 'crash' || tag == 'shutdown'"
    assert_equal(<<-DUMP, dump_plan(code))
[0]
  op:              <call>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  args[0]:         <#<proc:function in_values arguments:[]>>
  args[1]:         <#<column:var_size Events.tag range:ShortText type:scalar compress:none>>
  args[2]:         <"run">
  args[3]:         <"crash">
  args[4]:         <"shutdown">
  expr:            <0..5>
    DUMP
  end

  def test_combined
    code =
      "tag != 'error' && (tag == 'run' || tag == 'crash' || tag == 'shutdown')"
    assert_equal(<<-DUMP, dump_plan(code))
[0]
  op:              <not_equal>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <"error">
  expr:            <0..2>
[1]
  op:              <call>
  logical_op:      <and>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  args[0]:         <#<proc:function in_values arguments:[]>>
  args[1]:         <#<column:var_size Events.tag range:ShortText type:scalar compress:none>>
  args[2]:         <"run">
  args[3]:         <"crash">
  args[4]:         <"shutdown">
  expr:            <3..8>
    DUMP
  end
end
