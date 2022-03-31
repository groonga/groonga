module Groonga
  module Sharding
    class StreamExecuteContext
      attr_reader :enumerator
      attr_reader :filter
      attr_reader :post_filter
      attr_reader :dynamic_columns
      attr_reader :time_classify_types
      attr_reader :order
      def initialize(command_name, input)
        @input = input
        @enumerator = LogicalEnumerator.new(command_name,
                                            @input,
                                            {:unref_immediately => true})
        @filter = @input[:filter]
        @post_filter = @input[:post_filter]

        @dynamic_columns = DynamicColumns.parse("[#{command_name}]",
                                                @input)
        @temporary_tables_queue = []
        @time_classify_types = detect_time_classify_types
        @referred_objects_queue = []
        @order = parse_order(@input, :order)
      end

      def temporary_tables
        @temporary_tables_queue.last
      end

      def referred_objects
        @referred_objects_queue.last
      end

      def push
        @temporary_tables_queue << []
        @referred_objects_queue << []
      end

      def shift
        temporary_tables = @temporary_tables_queue.shift
        temporary_tables.each(&:close)
        temporary_tables.clear
        referred_objects = @referred_objects_queue.shift
        referred_objects.each(&:unref)
        referred_objects.clear
      end

      def close
        until @temporary_tables_queue.empty?
          shift
        end
        @enumerator.unref
      end

      def need_look_ahead?
        return false unless @dynamic_columns.have_window_function?
        return false unless @time_classify_types.empty?
        true
      end

      def parse_order(input, name)
        order = input[name]
        return :ascending if order.nil?

        case order
        when "ascending"
          :ascending
        when "descending"
          :descending
        else
          message =
            "[logical_range_filter] #{name} must be " +
            "\"ascending\" or \"descending\": <#{order}>"
          raise InvalidArgument, message
        end
      end

      def detect_time_classify_types
        window_group_keys = []
        @dynamic_columns.each_filtered do |dynamic_column|
          window_group_keys.concat(dynamic_column.window_group_keys)
        end
        return [] if window_group_keys.empty?

        types = []
        @dynamic_columns.each do |dynamic_column|
          next unless window_group_keys.include?(dynamic_column.label)
          case dynamic_column.value.strip
          when /\Atime_classify_(.+?)\s*\(\s*([a-zA-Z\d_]+)\s*[,)]/
            type = $1
            column = $2
            next if column != @enumerator.shard_key_name

            case type
            when "minute", "second"
              types << type
            when "day", "hour"
              types << type
            end
          end
        end
        types
      end
    end
  end
end
