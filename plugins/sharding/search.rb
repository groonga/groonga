module Groonga
  module Sharding
    class LogicalCountCommand < Command
      register("logical_count",
               [
                 "logical_table",
                 "shard_key",
                 "min",
                 "min_border",
                 "max",
                 "max_border",
                 "filter",
               ])

      def run_body(input)
        logical_table = input[:logical_table]
        if logical_table.nil?
          raise InvalidArgument, "[logical_count] logical_table is missing"
        end
        shard_key = input[:shard_key]
        if shard_key.nil?
          raise InvalidArgument, "[logical_count] shard_key is missing"
        end
        filter = input[:filter]

        total = 0
        target_range = TargetRange.new(input)
        each_table(logical_table) do |table, shard_range|
          total += count_n_records(table, filter,
                                   shard_key, shard_range, target_range)
        end
        output(total)
      end

      private
      def each_table(logical_table)
        prefix = "#{logical_table}_"
        context.database.each_table(:prefix => prefix) do |table|
          shard_range_raw = table.name[prefix.size..-1]

          next unless /\A(\d{4})(\d{2})(\d{2})\z/ =~ shard_range_raw
          shard_range = ShardRange.new($1.to_i, $2.to_i, $3.to_i)

          yield(table, shard_range)
        end
      end

      def count_n_records(table, filter, shard_key, shard_range, target_range)
        cover_type = target_range.cover_type(shard_range)
        case cover_type
        when :none
          0
        when :all
          if filter.nil?
            table.size
          else
            filtered_count_n_records(table, filter)
          end
        when :partial_min
          filtered_count_n_records(table, filter) do |expression|
            expression.append_constant(shard_key, Operator::PUSH, 1)
            expression.append_operator(Operator::GET_VALUE, 1)
            expression.append_constant(target_range.min, Operator::PUSH, 1)
            if target_range.min_border == :include
              expression.append_operator(Operator::GREATER_EQUAL, 2)
            else
              expression.append_operator(Operator::GREATER, 2)
            end
          end
        when :partial_max
          filtered_count_n_records(table, filter) do |expression|
            expression.append_constant(shard_key, Operator::PUSH, 1)
            expression.append_operator(Operator::GET_VALUE, 1)
            expression.append_constant(target_range.max, Operator::PUSH, 1)
            if target_range.max_border == :include
              expression.append_operator(Operator::LESS_EQUAL, 2)
            else
              expression.append_operator(Operator::LESS, 2)
            end
          end
        when :partial_min_and_max
          filtered_count_n_records(table, filter) do |expression|
            expression.append_object(context["between"], Operator::PUSH, 1)
            expression.append_constant(shard_key, Operator::PUSH, 1)
            expression.append_operator(Operator::GET_VALUE, 1)
            expression.append_constant(target_range.min, Operator::PUSH, 1)
            expression.append_constant(target_range.min_border,
                                       Operator::PUSH, 1)
            expression.append_constant(target_range.max, Operator::PUSH, 1)
            expression.append_constant(target_range.max_border,
                                       Operator::PUSH, 1)
            expression.append_operator(Operator::CALL, 5)
          end
        end
      end

      def filtered_count_n_records(table, filter)
        expression = nil
        filtered_table = nil

        begin
          expression = Expression.create(table)
          if block_given?
            yield(expression)
            if filter
              expression.parse(filter)
              expression.append_operator(Operator::AND, 2)
            end
          else
            expression.parse(filter)
          end
          filtered_table = table.select(expression)
          filtered_table.size
        ensure
          filtered_table.close if filtered_table
          expression.close if expression
        end
      end

      class ShardRange
        attr_reader :year, :month, :day
        def initialize(year, month, day)
          @year = year
          @month = month
          @day = day
        end
      end

      class TargetRange
        attr_reader :min, :min_border
        attr_reader :max, :max_border
        def initialize(input)
          @input = input
          @shard_key = input[:shard_key]
          @min = parse_value(:min)
          @min_border = parse_border(:min_border)
          @max = parse_value(:max)
          @max_border = parse_border(:max_border)
        end

        def cover_type(shard_range)
          return :all if @min.nil? and @max.nil?

          if @min and @max
            return :none unless in_min?(shard_range)
            return :none unless in_max?(shard_range)
            min_partial_p = in_min_partial?(shard_range)
            max_partial_p = in_max_partial?(shard_range)
            if min_partial_p and max_partial_p
              :partial_min_and_max
            elsif min_partial_p
              :partial_min
            elsif max_partial_p
              :partial_max
            else
              :all
            end
          elsif @min
            return :none unless in_min?(shard_range)
            if in_min_partial?(shard_range)
              :partial_min
            else
              :all
            end
          else
            return :none unless in_max?(shard_range)
            if in_max_partial?(shard_range)
              :partial_max
            else
              :all
            end
          end
        end

        private
        def parse_value(name)
          value = @input[name]
          return nil if value.nil?

          Converter.convert(value, Time)
        end

        def parse_border(name)
          border = @input[name]
          return :include if border.nil?

          case border
          when "include"
            :include
          when "exclude"
            :exclude
          else
            message =
              "[logical_count] #{name} must be \"include\" or \"exclude\": " +
              "<#{border}>"
            raise InvalidArgument, message
          end
        end

        def in_min?(shard_range)
          base_time = Time.local(shard_range.year,
                                 shard_range.month,
                                 shard_range.day + 1)
          @min < base_time
        end

        def in_min_partial?(shard_range)
          return false unless @min.year == shard_range.year
          return false unless @min.month == shard_range.month
          return false unless @min.day == shard_range.day

          return true if @min_border == :exclude

          @min.hour != 0 and
            @min.min != 0 and
            @min.sec != 0 and
            @min.usec != 0
        end

        def in_max?(shard_range)
          max_base_time = Time.local(shard_range.year,
                                     shard_range.month,
                                     shard_range.day)
          if @max_border == :include
            @max >= max_base_time
          else
            @max > max_base_time
          end
        end

        def in_max_partial?(shard_range)
          @max.year == shard_range.year and
            @max.month == shard_range.month and
            @max.day == shard_range.day
        end
      end
    end
  end
end
