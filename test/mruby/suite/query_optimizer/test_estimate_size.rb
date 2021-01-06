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
        schema.create_table("Hosts", key_type: :short_text) do |table|
        end

        schema.create_table("Logs") do |table|
          table.text("message")
          table.reference("host", "Hosts")
        end

        schema.create_table("Terms",
                            :type => :patricia_trie,
                            :default_tokenizer => "TokenBigramSplitSymbolAlpha",
                            :normalizer => "NormalizerAuto") do |table|
          table.index("Logs", "message")
          table.index("Hosts", "_key")
        end

        schema.change_table("Hosts") do |table|
          table.index("Logs", "host")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("message @ 'roonga'"))
    end

    def test_have_record
      @logs.add(:message => "Groonga is fast")
      @logs.add(:message => "Rroonga is fast")
      @logs.add(:message => "Mroonga is fast")
      assert_equal(3, estimate_size("message @ 'roonga'"))
    end

    def test_accessor
      @logs.add(:host => "www")
      @logs.add(:host => "mail")
      @logs.add(:host => "db1")
      @logs.add(:host => "db2")
      assert_equal(2, estimate_size("host._key @ 'w'"))
    end
  end

  class TestEqualSearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Hosts", key_type: :short_text) do |table|
        end

        schema.create_table("Logs") do |table|
          table.time("timestamp")
          table.reference("host", "Hosts")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end

        schema.change_table("Hosts") do |table|
          table.index("Logs", "host")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("timestamp == '2015-02-19 02:18:00'"))
    end

    def test_have_record
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      assert_equal(4, estimate_size("timestamp == '2015-02-19 02:18:00'"))
    end

    def test_accessor
      @logs.add(:host => "www")
      @logs.add(:host => "mail")
      @logs.add(:host => "db1")
      @logs.add(:host => "db2")
      assert_equal(1, estimate_size("host._key == 'mail'"))
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

  class TestPrefixSearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.short_text("user")
        end

        schema.create_table("Users",
                            :type => :patricia_trie,
                            :key_type => :short_text) do |table|
          table.index("Logs", "user")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("user @^ 'al'"))
    end

    def test_have_record
      @logs.add(:user => "bob")
      @logs.add(:user => "alice")
      @logs.add(:user => "alian")
      @logs.add(:user => "alice")
      @logs.add(:user => "alfread")
      @logs.add(:user => "calros")
      assert_equal(6, estimate_size("user @^ 'al'"))
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

  class TestInValuesSearch < self
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
                   estimate_size("in_values(timestamp, " +
                                 "'2015-02-19 02:18:00', " +
                                 "'2015-02-19 02:20:00')"))
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
      assert_equal(7,
                   estimate_size("in_values(timestamp, " +
                                 "'2015-02-19 02:18:00', " +
                                 "'2015-02-19 02:20:00')"))
    end
  end

  class TestQuerySearch < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Hosts", key_type: :short_text) do |table|
        end

        schema.create_table("Logs") do |table|
          table.text("message")
          table.reference("host", "Hosts")
        end

        schema.create_table("Terms",
                            :type => :patricia_trie,
                            :default_tokenizer => "TokenBigram",
                            :normalizer => "NormalizerAuto") do |table|
          table.index("Logs", "message")
          table.index("Hosts", "_key")
        end

        schema.change_table("Hosts") do |table|
          table.index("Logs", "host")
        end
      end
      super
    end

    def test_no_record
      assert_equal(0, estimate_size("query('message', 'shutdown')"))
    end

    def test_match_columns_column
      @logs.add(:message => "Start and shutdown")
      @logs.add(:message => "Shutdown")
      @logs.add(:message => "Running")
      assert_equal(3, estimate_size("query('message', 'shutdown')"))
    end

    def test_match_columns_index
      @logs.add(:message => "Start and shutdown")
      @logs.add(:message => "Shutdown")
      @logs.add(:message => "Running")
      assert_equal(3, estimate_size("query('Terms.Logs_message', 'shutdown')"))
    end

    def test_match_columns_scorer_column
      @logs.add(:message => "Start and shutdown")
      @logs.add(:message => "Shutdown")
      @logs.add(:message => "Running")
      assert_equal(3, estimate_size("query(" +
                                    "'scorer_tf_idf(message)', " +
                                    "'shutdown')"))
    end

    def test_match_columns_scorer_index
      @logs.add(:message => "Start and shutdown")
      @logs.add(:message => "Shutdown")
      @logs.add(:message => "Running")
      assert_equal(3, estimate_size("query(" +
                                    "'scorer_tf_idf(Terms.Logs_message)', " +
                                    "'shutdown')"))
    end
  end

  class TestAnd < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.text("message")
          table.time("timestamp")
          table.int32("type")
        end

        schema.create_table("Terms",
                            :type => :patricia_trie,
                            :default_tokenizer => "TokenBigramSplitSymbolAlpha",
                            :normalizer => "NormalizerAuto") do |table|
          table.index("Logs", "message")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end

        schema.create_table("Types",
                            :type => :patricia_trie,
                            :key_type => :int32) do |table|
          table.index("Logs", "type")
        end
      end
      super
    end

    def test_two
      @logs.add(:message => "Groonga is fast")
      @logs.add(:message => "Rroonga is fast")
      @logs.add(:message => "Mroonga is fast")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")

      assert_equal(3, estimate_size("message @ 'roonga' && " +
                                    "timestamp < '2015-02-19 02:19:00'"))
    end

    def test_three
      @logs.add(:message => "Groonga is fast", :type => 1)
      @logs.add(:message => "Rroonga is fast", :type => 1)
      @logs.add(:message => "Mroonga is fast", :type => 1)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)

      assert_equal(3, estimate_size("message @ 'roonga' && " +
                                    "timestamp < '2015-02-19 02:19:00' && " +
                                    "type == 1"))
    end
  end

  class TestAndNot < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.text("message")
          table.time("timestamp")
          table.int32("type")
        end

        schema.create_table("Terms",
                            :type => :patricia_trie,
                            :default_tokenizer => "TokenBigramSplitSymbolAlpha",
                            :normalizer => "NormalizerAuto") do |table|
          table.index("Logs", "message")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end

        schema.create_table("Types",
                            :type => :patricia_trie,
                            :key_type => :int32) do |table|
          table.index("Logs", "type")
        end
      end
      super
    end

    def test_two
      @logs.add(:message => "Groonga is fast")
      @logs.add(:message => "Rroonga is fast")
      @logs.add(:message => "Mroonga is fast")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")

      assert_equal(1, estimate_size("message @ 'roonga' &! " +
                                    "timestamp < '2015-02-19 02:19:00'"))
    end

    def test_three
      @logs.add(:message => "Groonga is fast", :type => 1)
      @logs.add(:message => "Rroonga is fast", :type => 1)
      @logs.add(:message => "Mroonga is fast", :type => 1)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)

      assert_equal(3, estimate_size("message @ 'roonga' &! " +
                                    "timestamp < '2015-02-19 02:19:00' &! " +
                                    "type == 1"))
    end
  end

  class TestOr < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.text("message")
          table.time("timestamp")
          table.int32("type")
        end

        schema.create_table("Terms",
                            :type => :patricia_trie,
                            :default_tokenizer => "TokenBigramSplitSymbolAlpha",
                            :normalizer => "NormalizerAuto") do |table|
          table.index("Logs", "message")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end

        schema.create_table("Types",
                            :type => :patricia_trie,
                            :key_type => :int32) do |table|
          table.index("Logs", "type")
        end
      end
      super
    end

    def test_two
      @logs.add(:message => "Groonga is fast")
      @logs.add(:message => "Rroonga is fast")
      @logs.add(:message => "Mroonga is fast")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:17:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:18:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")
      @logs.add(:timestamp => "2015-02-19 02:19:00")

      assert_equal(8, estimate_size("message @ 'roonga' || " +
                                    "timestamp < '2015-02-19 02:19:00'"))
    end

    def test_three
      @logs.add(:message => "Groonga is fast", :type => 1)
      @logs.add(:message => "Rroonga is fast", :type => 1)
      @logs.add(:message => "Mroonga is fast", :type => 1)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)

      assert_equal(8, estimate_size("message @ 'roonga' || " +
                                    "timestamp < '2015-02-19 02:19:00' || " +
                                    "type == 1"))
    end
  end

  class TestCompositeLogicalOperations < self
    def setup
      Groonga::Schema.define do |schema|
        schema.create_table("Logs") do |table|
          table.text("message")
          table.time("timestamp")
          table.int32("type")
        end

        schema.create_table("Terms",
                            :type => :patricia_trie,
                            :default_tokenizer => "TokenBigramSplitSymbolAlpha",
                            :normalizer => "NormalizerAuto") do |table|
          table.index("Logs", "message")
        end

        schema.create_table("Times",
                            :type => :patricia_trie,
                            :key_type => :time) do |table|
          table.index("Logs", "timestamp")
        end

        schema.create_table("Types",
                            :type => :patricia_trie,
                            :key_type => :int32) do |table|
          table.index("Logs", "type")
        end
      end
      super
    end

    def test_and_or
      @logs.add(:message => "Groonga is fast", :type => 1)
      @logs.add(:message => "Rroonga is fast", :type => 1)
      @logs.add(:message => "Mroonga is fast", :type => 1)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)

      assert_equal(3, estimate_size("(" +
                                    "message @ 'roonga' && " +
                                    "timestamp < '2015-02-19 02:19:00'" +
                                    ") || " +
                                    "(" +
                                    "message @ 'roonga' && " +
                                    "timestamp < '2015-02-19 02:19:00' && " +
                                    "type == 1" +
                                    ")"))
    end

    def test_or_and
      @logs.add(:message => "Groonga is fast", :type => 1)
      @logs.add(:message => "Rroonga is fast", :type => 1)
      @logs.add(:message => "Mroonga is fast", :type => 1)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:17:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:18:00", :type => 2)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)
      @logs.add(:timestamp => "2015-02-19 02:19:00", :type => 3)

      assert_equal(1,
                   estimate_size("(" +
                                 "timestamp < '2015-02-19 02:19:00' || " +
                                 "type == 2 " +
                                 ") && " +
                                 "(" +
                                 "message @ 'Groonga' || " +
                                 "message @ 'Rroonga'" +
                                 ")"))
    end
  end
end
