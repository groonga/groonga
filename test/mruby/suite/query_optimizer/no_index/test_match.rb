class TestNoIndexMatch < QueryOptimizerTestCase
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

  def test_only
    assert_equal(<<-DUMP, dump_plan("message @ 'Groonga'"))
[0]
  op:              <match>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <"Groonga">
  expr:            <0..2>
    DUMP
  end

  def test_with_arithmetic_operator
    assert_equal(<<-DUMP, dump_plan("message @ 'Groonga' && ((1 + 1) == 2)"))
[0]
  op:              <match>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <"Groonga">
  expr:            <0..2>
[1]
  op:              <equal>
  logical_op:      <and>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <2>
  expr:            <3..7>
    DUMP
  end

  def test_not
    assert_equal(<<-DUMP, dump_plan("!(message @ 'Groonga')"))
sequential search
    DUMP
  end
end
