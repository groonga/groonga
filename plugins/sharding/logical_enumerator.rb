module Groonga
  module Sharding
    class LogicalEnumerator
      include Enumerable

      attr_reader :target_range
      attr_reader :logical_table
      attr_reader :shard_key_name
      def initialize(command_name, input, options={})
        @command_name = command_name
        @input = input
        @options = options
        initialize_parameters
      end

      def each(&block)
        each_internal(:ascending, &block)
      end

      def reverse_each(&block)
        each_internal(:descending, &block)
      end

      private
      def each_internal(order)
        @strategy.each(order) do |shard, shard_range|
          yield(shard, shard_range)
        end
      end

      def initialize_parameters
        @logical_table = @input[:logical_table]
        if @logical_table.nil?
          raise InvalidArgument, "[#{@command_name}] logical_table is missing"
        end

        @shard_key_name = @input[:shard_key]
        if @shard_key_name.nil?
          require_shard_key = @options[:require_shard_key]
          require_shard_key = true if require_shard_key.nil?
          if require_shard_key
            raise InvalidArgument, "[#{@command_name}] shard_key is missing"
          end
        end

        prefix = "#{@logical_table}_"
        @strategy_name = @input[:strategy] || "date-range"
        case @strategy_name
        when "date-range"
          @strategy = DateStrategy.new(prefix, @shard_key_name)
        when "uint-range"
          @strategy = UIntStrategy.new(prefix, @shard_key_name)
        else
          message = "[#{@command_name}] strategy must be " +
            "date-range or uint-ragne: #{@strategy_name}"
          raise InvalidArgument, message
        end

        @target_range = TargetRange.new(@command_name, @strategy, @input)
      end

      class Strategy
        def initialize(prefix, shard_key_name)
          @prefix = prefix
          @shard_key_name = shard_key_name
          @context = Context.instance
        end
      end

      class DateStrategy < Strategy
        def parse_value(value)
          Converter.convert(value, Time)
        end

        def each(order)
          each_shard_with_around(order) do |prev_shard, current_shard, next_shard|
            shard_range_data = current_shard.range_data
            shard_range = nil

            if shard_range_data.day.nil?
              if order == :ascending
                if next_shard
                  next_shard_range_data = next_shard.range_data
                else
                  next_shard_range_data = nil
                end
              else
                if prev_shard
                  next_shard_range_data = prev_shard.range_data
                else
                  next_shard_range_data = nil
                end
              end
              max_day = compute_month_shard_max_day(shard_range_data.year,
                                                    shard_range_data.month,
                                                    next_shard_range_data)
              shard_range = MonthShardRange.new(shard_range_data.year,
                                                shard_range_data.month,
                                                max_day)
            else
              shard_range = DayShardRange.new(shard_range_data.year,
                                              shard_range_data.month,
                                              shard_range_data.day)
            end

            yield(current_shard, shard_range)
          end
        end

        private
        def each_shard_with_around(order)
          shards = [nil]
          @context.database.each_name(:prefix => @prefix,
                                      :order_by => :key,
                                      :order => order) do |name|
            shard_range_raw = name[@prefix.size..-1]

            case shard_range_raw
            when /\A(\d{4})(\d{2})\z/
              shard_range_data = DateShardRangeData.new($1.to_i, $2.to_i, nil)
            when /\A(\d{4})(\d{2})(\d{2})\z/
              shard_range_data = DateShardRangeData.new($1.to_i, $2.to_i, $3.to_i)
            else
              next
            end

            shards << Shard.new(name, @shard_key_name, shard_range_data)
            next if shards.size < 3
            yield(*shards)
            shards.shift
          end

          if shards.size == 2
            yield(shards[0], shards[1], nil)
          end
        end

        def compute_month_shard_max_day(year, month, next_shard_range)
          return nil if next_shard_range.nil?

          return nil if month != next_shard_range.month

          next_shard_range.day
        end
      end

      class UIntStrategy < Strategy
        def parse_value(value)
          Converter.convert(value, Fixnum)
        end

        def each(order)
          each_shard_with_around(order) do |prev_shard, current_shard, next_shard|
            min = current_shard.range_data.value
            if next_shard
              max = next_shard.range_data.value
            else
              max = nil
            end
            shard_range = UIntShardRange.new(min, max)
            yield(current_shard, shard_range)
          end
        end

        private
        def each_shard_with_around(order, &block)
          shards = []
          @context.database.each_name(:prefix => @prefix) do |name|
            shard_range_raw = name[@prefix.size..-1]
            case shard_range_raw
            when /\A(\d+)\z/
              shard_range_data = UIntShardRangeData.new($1.to_i)
              shards << Shard.new(name, @shard_key_name, shard_range_data)
            end
          end
          return if shards.empty?

          sorted_shards = shards.sort_by do |shard|
            value = shard.range_data.value
            value = -value if order == :descending
            value
          end
          sorted_shards.unshift(nil)
          sorted_shards << nil
          sorted_shards.each_cons(3, &block)
        end
      end

      class Shard
        attr_reader :table_name, :key_name, :range_data
        def initialize(table_name, key_name, range_data)
          @table_name = table_name
          @key_name = key_name
          @range_data = range_data
        end

        def table
          @table ||= Context.instance[@table_name]
        end

        def full_key_name
          "#{@table_name}.#{@key_name}"
        end

        def key
          @key ||= Context.instance[full_key_name]
        end
      end

      class DateShardRangeData
        attr_reader :year, :month, :day
        def initialize(year, month, day)
          @year = year
          @month = month
          @day = day
        end

        def to_suffix
          if @day.nil?
            "_%04d%02d" % [@year, @month]
          else
            "_%04d%02d%02d" % [@year, @month, @day]
          end
        end
      end

      class UIntShardRangeData
        attr_reader :value
        def initialize(value)
          @value = value
        end

        def to_suffix
          "_%d" % [@value]
        end
      end

      class DayShardRange
        attr_reader :year, :month, :day
        def initialize(year, month, day)
          @year = year
          @month = month
          @day = day
        end

        def least_over_value
          next_day = Time.local(@year, @month, @day) + (60 * 60 * 24)
          while next_day.day == @day # For leap second
            next_day += 1
          end
          next_day
        end

        def min
          Time.local(@year, @month, @day)
        end

        def include?(time)
          @year == time.year and
            @month == time.month and
            @day == time.day
        end
      end

      class MonthShardRange
        attr_reader :year, :month, :max_day
        def initialize(year, month, max_day)
          @year = year
          @month = month
          @max_day = max_day
        end

        def least_over_value
          if @max_day.nil?
            if @month == 12
              Time.local(@year + 1, 1, 1)
            else
              Time.local(@year, @month + 1, 1)
            end
          else
            Time.local(@year, @month, @max_day)
          end
        end

        def min
          Time.local(@year, @month, 1)
        end

        def include?(time)
          return false unless @year == time.year
          return false unless @month == time.month

          if @max_day.nil?
            true
          else
            time.day <= @max_day
          end
        end
      end

      class UIntShardRange
        attr_reader :min, :max
        def initialize(min, max)
          @min = min
          @max = max
        end

        def least_over_value
          @max
        end

        def include?(value)
          if value < @min
            false
          else
            if @max.nil?
              true
            else
              value < @max
            end
          end
        end
      end

      class TargetRange
        attr_reader :min, :min_border
        attr_reader :max, :max_border
        def initialize(command_name, strategy, input)
          @command_name = command_name
          @strategy = strategy
          @input = input
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

          @strategy.parse_value(value)
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
              "[#{@command_name}] #{name} must be \"include\" or \"exclude\": " +
              "<#{border}>"
            raise InvalidArgument, message
          end
        end

        def in_min?(shard_range)
          @min < shard_range.least_over_value
        end

        def in_min_partial?(shard_range)
          return false unless shard_range.include?(@min)

          return true if @min_border == :exclude

          shard_range.min != @min
        end

        def in_max?(shard_range)
          max_base_time = shard_range.min
          if @max_border == :include
            @max >= max_base_time
          else
            @max > max_base_time
          end
        end

        def in_max_partial?(shard_range)
          shard_range.include?(@max)
        end
      end
    end
  end
end
