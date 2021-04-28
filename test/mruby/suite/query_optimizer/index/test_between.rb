class TestBetween < QueryOptimizerTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("Logs") do |table|
        table.time("timestamp")
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

  def test_neighbor
    code =
      "timestamp >= '2015-10-01 00:00:00' && " +
      "timestamp <  '2015-11-00 00:00:00'"
    assert_equal(<<-DUMP, dump_plan(code))
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
    assert_equal(<<-DUMP, dump_plan(code))
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
  index:      <[]>
  query:      <1>
  expr:       <7..9>
[2]
  op:         <push>
  logical_op: <and>
  index:      <[]>
  query:      <(NULL)>
  expr:       <11..11>
    DUMP
  end

  def test_with_function_call
    code =
      "between(timestamp, '2015-09-01 00:00:00', 'include', '2015-10-15 00:00:00', 'include') && " +
      "timestamp >= '2015-10-01 00:00:00' && " +
      "timestamp <  '2015-11-00 00:00:00'"
    assert_equal(<<-DUMP, dump_plan(code))
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

  def test_with_index_column
    code =
      "Terms.Logs_message @ 'Groonga' && " +
      "timestamp >= '2015-10-01 00:00:00' && " +
      "timestamp <  '2015-11-00 00:00:00'"
    assert_equal(<<-DUMP, dump_plan(code))
[0]
  op:         <match>
  logical_op: <or>
  index:      <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:      <\"Groonga\">
  expr:       <0..2>
[1]
  op:         <call>
  logical_op: <and>
  args[0]:    <#<proc:function between arguments:[]>>
  args[1]:    <#<column:fix_size Logs.timestamp range:Time type:scalar compress:none>>
  args[2]:    <\"2015-10-01 00:00:00\">
  args[3]:    <\"include\">
  args[4]:    <\"2015-11-00 00:00:00\">
  args[5]:    <\"exclude\">
  expr:       <3..9>
    DUMP
  end

  def test_with_accessor
    code =
      "Terms.Logs_message.message @ 'Groonga' && " +
      "timestamp >= '2015-10-01 00:00:00' && " +
      "timestamp <  '2015-11-00 00:00:00'"
    assert_equal(<<-DUMP, dump_plan(code))
[0]
  op:         <match>
  logical_op: <or>
  index:      <[#<accessor Logs_message(Terms).message(Logs)>]>
  query:      <\"Groonga\">
  expr:       <0..2>
[1]
  op:         <call>
  logical_op: <and>
  args[0]:    <#<proc:function between arguments:[]>>
  args[1]:    <#<column:fix_size Logs.timestamp range:Time type:scalar compress:none>>
  args[2]:    <\"2015-10-01 00:00:00\">
  args[3]:    <\"include\">
  args[4]:    <\"2015-11-00 00:00:00\">
  args[5]:    <\"exclude\">
  expr:       <3..9>
    DUMP
  end

  def test_with_options
    code =
      "fuzzy_search(message, 'roonga', {\"max_distance\": 1}) && " +
      "timestamp >= '2015-10-01 00:00:00' && " +
      "timestamp <  '2015-11-00 00:00:00'"
    assert_equal(<<-DUMP, dump_plan(code))
[0]
  op:         <call>
  logical_op: <or>
  args[0]:    <#<proc:function fuzzy_search arguments:[]>>
  args[1]:    <#<column:var_size Logs.message range:Text type:scalar compress:none>>
  args[2]:    <\"roonga\">
  args[3]:    <#<table:hash (nil) key:ShortText value:(nil) size:1 columns:[] default_tokenizer:(nil) normalizer:(nil) keys:[\"max_distance\"] subrec:none>>
  expr:       <0..4>
[1]
  op:         <call>
  logical_op: <and>
  args[0]:    <#<proc:function between arguments:[]>>
  args[1]:    <#<column:fix_size Logs.timestamp range:Time type:scalar compress:none>>
  args[2]:    <\"2015-10-01 00:00:00\">
  args[3]:    <\"include\">
  args[4]:    <\"2015-11-00 00:00:00\">
  args[5]:    <\"exclude\">
  expr:       <5..11>
    DUMP
  end
end
