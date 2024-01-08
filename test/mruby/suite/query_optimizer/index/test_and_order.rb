class TestAndOrder < QueryOptimizerTestCase
  def setup
    Groonga::Schema.define do |schema|
      schema.create_table("Logs") do |table|
        table.time("created_at")
        table.time("updated_at")
      end

      schema.create_table("Timestamps",
                          :type => :patricia_trie,
                          :key_type => :time) do |table|
        table.index("Logs.created_at")
        table.index("Logs.updated_at")
      end
    end

    @logs = Groonga["Logs"]
    setup_logs
    setup_expression(@logs)
  end

  def setup_logs
    100.times do
      @logs.add(:created_at => "2015-10-01 00:00:00",
                :updated_at => "2015-10-01 00:00:00")
    end

    50.times do
      @logs.add(:created_at => "2015-10-02 00:00:00",
                :updated_at => "2015-10-02 00:00:00")
    end

    10.times do
      @logs.add(:created_at => "2015-10-03 00:00:00",
                :updated_at => "2015-10-03 00:00:00")
    end
  end

  def teardown
    teardown_expression
  end

  def test_range
    code =
      "created_at <= '2015-10-01 00:00:00' && " +
      "updated_at >= '2015-10-03 00:00:00'"
    assert_equal(<<-DUMP, dump_plan(code))
[0]
  op:              <greater_equal>
  logical_op:      <or>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Timestamps.Logs_updated_at range:Logs sources:[Logs.updated_at] flags:NONE>]>
  query:           <"2015-10-03 00:00:00">
  expr:            <0..2>
[1]
  op:              <less_equal>
  logical_op:      <and>
  weight_factor:   <1.000000>
  sections:        <[0]>
  weights:         <[1.0]>
  start_positions: <[-1]>
  index:           <[#<column:index Timestamps.Logs_created_at range:Logs sources:[Logs.created_at] flags:NONE>]>
  query:           <"2015-10-01 00:00:00">
  expr:            <3..5>
    DUMP
  end
end
