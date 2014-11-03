class TestIndex < QueryOptimizerTestCase
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
    @expression = create_expression(@logs)
  end

  def teardown
    @expression.close
  end

  def test_match
    assert_equal(<<-DUMP, dump_plan(@expression, "message @ 'Groonga'"))
[0]
  op:         <match>
  logical_op: <or>
  query:      <"Groonga">
  expr:       <0..2>
    DUMP
  end
end
