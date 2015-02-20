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

  class TestLessSearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.time("timestamp")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("timestamp < '2015-02-19 02:19:00'"))
    end

    def test_have_record
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      assert_equal(8, estimate_size("timestamp < '2015-02-19 02:19:00'"))
    end
  end

  class TestLessEqualSearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.time("timestamp")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("timestamp <= '2015-02-19 02:18:00'"))
    end

    def test_have_record
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      assert_equal(8, estimate_size("timestamp <= '2015-02-19 02:18:00'"))
    end
  end

  class TestGreaterSearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.time("timestamp")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("timestamp > '2015-02-19 02:17:00'"))
    end

    def test_have_record
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      assert_equal(8, estimate_size("timestamp > '2015-02-19 02:17:00'"))
    end
  end

  class TestGreaterEqualSearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.time("timestamp")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("timestamp >= '2015-02-19 02:18:00'"))
    end

    def test_have_record
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      assert_equal(8, estimate_size("timestamp >= '2015-02-19 02:18:00'"))
    end
  end

  class TestBetweenSearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.time("timestamp")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0,
                   estimate_size("between(timestamp, " +
                                 "'2015-02-19 02:18:00', 'include', " +
                                 "'2015-02-19 02:20:00', 'exclude')"))
    end

    def test_have_record
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:20:00")
      @logs.add(:timestamp => "2015-02-19 02:20:00")
      @logs.add(:timestamp => "2015-02-19 02:21:00")
      @logs.add(:timestamp => "2015-02-19 02:21:00")
      assert_equal(12,
                   estimate_size("between(timestamp, " +
                                 "'2015-02-19 02:18:00', 'include', " +
                                 "'2015-02-19 02:20:00', 'exclude')"))
    end
  end
end
