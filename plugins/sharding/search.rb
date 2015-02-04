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
        filter = input[:filter]

        total = 0
        each_table(input) do |table|
          total += count_n_records(table, filter)
        end
        output(total)
      end

      private
      def each_table(input)
        logical_table = input[:logical_table]
        if logical_table.nil?
          raise InvalidArgument, "[logical_count] logical_table is missing"
        end

        prefix = "#{logical_table}_"
        target_range = TargetRange.new(input)
        context.database.each_table(:prefix => prefix) do |table|
          shard_key = table.name[prefix.size..-1]
          next unless target_range.include?(shard_key)
          yield(table)
        end
      end

      def count_n_records(table, filter)
        return table.size if filter.nil?

        expression = nil
        filtered_table = nil
        begin
          expression = Expression.create(table)
          expression.parse(filter)
          filtered_table = table.select(expression)
          filtered_table.size
        ensure
          filtered_table.close if filtered_table
          expression.close if expression
        end
      end

      class TargetRange
        def initialize(input)
          @input = input
          @min = parse_value(:min)
          @min_border = parse_border(:min_border)
          @max = parse_value(:max)
          @max_border = parse_border(:max_border)
        end

        def include?(shard_key)
          if /\A(\d{4})(\d{2})(\d{2})\z/ =~ shard_key
            year = $1.to_i
            month = $2.to_i
            day = $3.to_i
            in_range?(year, month, day)
          else
            false
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

        def in_range?(year, month, day)
          in_min?(year, month, day) and in_max?(year, month, day)
        end

        def in_min?(year, month, day)
          return true if @min.nil?

          min_base_time = Time.local(year, month, day + 1)
          @min < min_base_time
        end

        def in_max?(year, month, day)
          return true if @max.nil?

          max_base_time = Time.local(year, month, day)
          if @max_border == :include
            @max >= max_base_time
          else
            @max > max_base_time
          end
        end
      end
    end
  end
end
