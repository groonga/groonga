class TestEstimateSize < QueryOptimizerTestCase
  def setup
    @logs = Groonga["Logs"]
    setup_expression(@logs)
  end

  def teardown
    teardown_expression
  end

  private
  def estimate_size(text, options={})
    parse(text, options)
    @expression.estimate_size
  end

  class TestFullTextSearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.text("message")
        end

        schema.create_table("Terms",
                            :type => :patricia_trie,
                            :default_tokenizer => "TokenBigramSplitSymbolAlpha",
                            :normalizer => "NormalizerAuto") do |table|
          table.index("Logs", "message")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("message @ 'Groonga'"))
    end

    def test_have_record
      @logs.add(:message => "Groonga is fast")
      @logs.add(:message => "Rroonga is fast")
      @logs.add(:message => "Mroonga is fast")
      assert_equal(6, estimate_size("message @ 'Groonga'"))
    end
  end
end
