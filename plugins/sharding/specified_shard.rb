module Groonga
  module Sharding
    class SpecifiedShard
      class << self
        def parse(tag, input)
          shards = []
          labeled_arguments = LabeledArguments.new(input, /shards?/)
          labeled_arguments.each do |label, arguments|
            shards << new(tag, label, arguments)
          end

          # Numeric labels such as 1, 2 and 10 are sorted numerically.
          # Other labels are sorted as strings.
          numeric = shards.all? do |shard|
            /\A\d+\z/.match?(shard.label)
          end
          if numeric
            shards.sort_by {|shard| shard.label.to_i}
          else
            shards.sort_by(&:label)
          end
        end
      end

      attr_reader :label
      attr_reader :table_name
      def initialize(tag, label, arguments)
        @tag = tag
        @label = label
        @table_name = arguments["table"]
        validate_table_name
      end

      private
      def validate_table_name
        if @table_name.nil?
          raise InvalidArgument, "#{error_message_tag} table is missing"
        end

        table = Context.instance[@table_name]
        begin
          unless table
            raise InvalidArgument, "#{error_message_tag} table doesn't exist: <#{@table_name}>"
          end
          unless table.is_a?(Table)
            raise InvalidArgument, "#{error_message_tag} table must be a table: <#{@table_name}>"
          end
        ensure
          table.unref if table
        end

        true
      end

      def error_message_tag
        "#{@tag}[shards][#{@label}]"
      end
    end
  end
end
