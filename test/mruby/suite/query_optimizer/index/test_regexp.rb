class TestRegexp < QueryOptimizerTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("Logs") do |table|
        table.text("message")
      end

      schema.create_table("Terms",
                          :type => :patricia_trie,
                          :default_tokenizer => "TokenRegexp",
                          :normalizer => "NormalizerAuto") do |table|
        table.index("Logs", "message")
      end
    end

    @logs = Groonga["Logs"]
    setup_logs
    setup_expression(@logs)
  end

  def setup_logs
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

  def teardown
    teardown_expression
  end

  def test_and
    filter = "(message @~ 'Groonga') && (message @~ 'Rroonga')"
    assert_equal(<<-DUMP, dump_plan(filter))
[0]
  op:              <regexp>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Terms.Logs_message range:Logs sources:[Logs.message] flags:POSITION>]>
  query:           <"Rroonga">
  expr:            <0..2>
[1]
  op:              <regexp>
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
end
