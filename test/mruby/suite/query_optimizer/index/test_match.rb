class TestIndexMatch < QueryOptimizerTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("Logs") do |table|
        table.text("message")
      end

      schema.create_table("Terms",
                          :type => :patricia_trie,
                          :default_tokenizer => "TokenBigram",
                          :normalizer => "NormalizerAuto") do |table|
        table.index("Logs", "message")
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
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
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
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
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
[0]
  op:              <call>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  args[0]:         <#<proc:function all_records arguments:[]>>
  expr:            <0..0>
[1]
  op:              <match>
  logical_op:      <and_not>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Groonga">
  expr:            <0..2>
    DUMP
  end

  def test_not_and_normal
    assert_equal(<<-DUMP, dump_plan("!(message @ 'Groonga') && (message @ 'Rroonga')"))
[0]
  op:              <call>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  args[0]:         <#<proc:function all_records arguments:[]>>
  expr:            <0..0>
[1]
  op:              <match>
  logical_op:      <and_not>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Groonga">
  expr:            <0..2>
[2]
  op:              <match>
  logical_op:      <and>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Rroonga">
  expr:            <4..6>
    DUMP
  end

  def test_normal_and_not
    assert_equal(<<-DUMP, dump_plan("(message @ 'Rroonga') && !(message @ 'Groonga')"))
[0]
  op:              <match>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Rroonga">
  expr:            <0..2>
[1]
  op:              <match>
  logical_op:      <and_not>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Groonga">
  expr:            <3..5>
    DUMP
  end

  def test_normal_and_not_not
    assert_equal(<<-DUMP, dump_plan("(message @ 'Rroonga') &! !(message @ 'Groonga')"))
[0]
  op:              <match>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Rroonga">
  expr:            <0..2>
[1]
  op:              <match>
  logical_op:      <and>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Groonga">
  expr:            <3..5>
    DUMP
  end

  def test_normal_or_not
    assert_equal(<<-DUMP, dump_plan("(message @ 'Rroonga') || !(message @ 'Groonga')"))
[0]
  op:              <call>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[]>
  weights:         <[]>
  start_positions: <[]>
  args[0]:         <#<proc:function all_records arguments:[]>>
  expr:            <0..0>
[1]
  op:              <match>
  logical_op:      <and_not>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Groonga">
  expr:            <3..5>
[2]
  op:              <match>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Rroonga">
  expr:            <0..2>
    DUMP
  end

  sub_test_case("optimization by estimate size") do
    def setup
      super
      10.times do |i|
        @logs.add(:message => "Groonga#{i}")
      end
      2.times do |i|
        @logs.add(:message => "Rroonga#{i}")
      end
      100.times do |i|
        @logs.add(:message => "Mroonga#{i}")
      end
    end

    def test_and
      filter = "(message @ 'Groonga') && (message @ 'Rroonga')"
      assert_equal(<<-DUMP, dump_plan(filter))
[0]
  op:              <match>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Rroonga">
  expr:            <0..2>
[1]
  op:              <match>
  logical_op:      <and>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Groonga">
  expr:            <3..5>
      DUMP
    end

    def test_and_and_not
      filter = "(message @ 'Groonga') && "
      filter << "(message @ 'Rroonga') &! "
      filter << "(message @ 'Mroonga')"
      assert_equal(<<-DUMP, dump_plan(filter))
[0]
  op:              <match>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Rroonga">
  expr:            <0..2>
[1]
  op:              <match>
  logical_op:      <and>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Groonga">
  expr:            <3..5>
[2]
  op:              <match>
  logical_op:      <and_not>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Mroonga">
  expr:            <7..9>
      DUMP
    end
  end
end
