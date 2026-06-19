class TestNoIndexCompare < QueryOptimizerTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("Values") do |table|
        table.int32("number")
      end
    end

    @values = Groonga["Values"]
    setup_expression(@values)
  end

  def teardown
    teardown_expression
  end

  def test_less
    assert_equal(<<-DUMP, dump_plan("number < 29"))
[0]
  op:              <less>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <0..2>
    DUMP
  end

  def test_not_less
    assert_equal(<<-DUMP, dump_plan("!(number < 29)"))
[0]
  op:              <greater_equal>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <0..3>
    DUMP
  end

  def test_not_less_equal
    assert_equal(<<-DUMP, dump_plan("!(number <= 29)"))
[0]
  op:              <greater>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <0..3>
    DUMP
  end

  def test_not_greater
    assert_equal(<<-DUMP, dump_plan("!(number > 29)"))
[0]
  op:              <less_equal>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <0..3>
    DUMP
  end

  def test_not_greater_equal
    assert_equal(<<-DUMP, dump_plan("!(number >= 29)"))
[0]
  op:              <less>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <0..3>
    DUMP
  end

  def test_not_equal
    assert_equal(<<-DUMP, dump_plan("!(number == 29)"))
[0]
  op:              <not_equal>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <0..3>
    DUMP
  end

  def test_not_not_equal
    assert_equal(<<-DUMP, dump_plan("!(number != 29)"))
[0]
  op:              <equal>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <0..3>
    DUMP
  end

  sub_test_case("multiple conditions") do
    test("not at the first") do
      assert_equal(<<-DUMP, dump_plan("!(number < 29) && _id == 29"))
[0]
  op:              <greater_equal>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <0..3>
[1]
  op:              <equal>
  logical_op:      <and>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<accessor _id(Values)>]>
  query:           <29>
  expr:            <4..6>
      DUMP
    end

    test("not at the last") do
      assert_equal(<<-DUMP, dump_plan("_id == 29 && !(number < 29)"))
[0]
  op:              <equal>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<accessor _id(Values)>]>
  query:           <29>
  expr:            <0..2>
[1]
  op:              <less>
  logical_op:      <and_not>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  index:           <[]>
  query:           <29>
  expr:            <3..5>
      DUMP
    end
  end
end
