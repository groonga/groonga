module Groonga
  module Sharding
    class LogicalRangeFilterCommand < Command
      register("logical_range_filter",
               [
                 "logical_table",
                 "shard_key",
                 "min",
                 "min_border",
                 "max",
                 "max_border",
                 "order",
                 "filter",
                 "offset",
                 "limit",
                 "output_columns",
                 "use_range_index",
                 "post_filter",
                 "sort_keys",
               ])

      def run_body(input)
        output_columns = input[:output_columns] || "_key, *"
        is_stream_output = stream_output?(input)

        context = ExecuteContext.new(input)
        begin
          limit = context.limit
          if limit < 0 and limit != -1
            message =
              "[logical_range_filter] negative limit must be -1: #{limit}"
            raise InvalidArgument, message
          end

          n_elements = 0
          is_first = true
          executor = Executor.new(context)
          executor.execute do |result_set|
            n_elements += result_set.size

            if is_first
              if is_stream_output
                writer.open_map("RESULTSET", -1)
              else
                writer.open_array("RESULTSET", -1)
              end
              writer.write_table_columns(result_set, output_columns)
              writer.open_table_records(-1)
              is_first = false
            end

            options = {}
            options[:limit] = limit
            options[:auto_flush] = true if is_stream_output
            writer.write_table_records_content(result_set,
                                               output_columns,
                                               options)
            writer.flush if is_stream_output
            if limit != -1
              limit -= result_set.size
              break if limit <= 0
            end
          end
          unless is_first
            writer.close_table_records
            if is_stream_output
              writer.close_map
            else
              writer.close_array
            end
          end
          query_logger.log(:size, ":", "output(#{n_elements})")
        ensure
          context.close
        end
      end

      private
      def stream_output?(input)
        context.command_version >= 3
      end

      def cache_key(input)
        return nil if stream_output?(input)

        key = "logical_range_filter\0"
        key << "#{input[:logical_table]}\0"
        key << "#{input[:shard_key]}\0"
        key << "#{input[:min]}\0"
        key << "#{input[:min_border]}\0"
        key << "#{input[:max]}\0"
        key << "#{input[:max_border]}\0"
        key << "#{input[:order]}\0"
        key << "#{input[:filter]}\0"
        key << "#{input[:offset]}\0"
        key << "#{input[:limit]}\0"
        key << "#{input[:output_columns]}\0"
        key << "#{input[:use_range_index]}\0"
        key << "#{input[:post_filter]}\0"
        key << "#{input[:sort_keys]}\0"
        key << "#{context.command_version}\0"
        key << "#{context.output_type}\0"
        dynamic_columns = DynamicColumns.parse("[logical_range_filter]",
                                               input)
        key << dynamic_columns.cache_key
        key
      end

      class ExecuteContext
        include KeysParsable

        attr_reader :use_range_index
        attr_reader :enumerator
        attr_reader :order
        attr_reader :filter
        attr_reader :offset
        attr_reader :limit
        attr_reader :post_filter
        attr_reader :sort_keys
        attr_reader :dynamic_columns
        attr_accessor :current_offset
        attr_accessor :current_limit
        attr_reader :threshold
        attr_reader :large_shard_threshold
        attr_reader :time_classify_types
        attr_reader :result_sets
        attr_reader :referred_objects
        def initialize(input)
          @input = input
          @use_range_index = parse_use_range_index(@input[:use_range_index])
          @enumerator = LogicalEnumerator.new("logical_range_filter", @input)
          @order = parse_order(@input, :order)
          @filter = @input[:filter]
          @offset = (@input[:offset] || 0).to_i
          @limit = (@input[:limit] || 10).to_i
          @post_filter = @input[:post_filter]
          @sort_keys = parse_keys(@input[:sort_keys])

          @dynamic_columns = DynamicColumns.parse("[logical_range_filter]",
                                                  @input)

          @current_offset = @offset
          @current_limit = @limit

          @result_sets = []
          @temporary_tables_stack = []

          @threshold = compute_threshold
          @large_shard_threshold = compute_large_shard_threshold

          @time_classify_types = detect_time_classify_types

          @referred_objects = []
        end

        def temporary_tables
          @temporary_tables_stack.last
        end

        def push
          @temporary_tables_stack << []
        end

        def shift
          temporary_tables = @temporary_tables_stack.shift
          temporary_tables.each do |table|
            table.close
          end
          temporary_tables.clear
        end

        def close
          until @temporary_tables_stack.empty?
            shift
          end
          @result_sets.clear
          @referred_objects.each(&:unref)
          @enumerator.unref
        end

        def consume_result_sets
          while (result_set = @result_sets.shift)
            yield(result_set)
          end
        end

        def need_look_ahead?
          return false unless @dynamic_columns.have_window_function?
          return false unless @time_classify_types.empty?
          true
        end

        private
        def parse_use_range_index(use_range_index)
          case use_range_index
          when "yes"
            true
          when "no"
            false
          else
            nil
          end
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

        def compute_threshold
          threshold_env = ENV["GRN_LOGICAL_RANGE_FILTER_THRESHOLD"]
          default_threshold = 0.3
          (threshold_env || default_threshold).to_f
        end

        def compute_large_shard_threshold
          threshold_env = ENV["GRN_LOGICAL_RANGE_FILTER_LARGE_SHARD_THRESHOLD"]
          default_threshold = "10_000"
          (threshold_env || default_threshold).to_i(10)
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

      class Executor
        def initialize(context)
          @context = context
        end

        def execute
          first_shard = nil
          have_result_set = false
          each_shard_executor do |shard_executor|
            first_shard ||= shard_executor.shard
            shard_executor.execute
            @context.consume_result_sets do |result_set|
              have_result_set = true
              yield(result_set)
            end
            break if @context.current_limit.zero?
          end
          if first_shard.nil?
            enumerator = @context.enumerator
            message =
              "[logical_range_filter] no shard exists: " +
              "logical_table: <#{enumerator.logical_table}>: " +
              "shard_key: <#{enumerator.shard_key_name}>"
            raise InvalidArgument, message
          end
          unless have_result_set
            result_set = HashTable.create(:flags => ObjectFlags::WITH_SUBREC,
                                          :key_type => first_shard.table)
            @context.push
            @context.temporary_tables << result_set
            targets = [[result_set]]
            @context.dynamic_columns.apply_initial(targets)
            @context.dynamic_columns.apply_filtered(targets)
            yield(result_set)
            @context.shift
          end
        end

        private
        def each_shard_executor(&block)
          enumerator = @context.enumerator
          target_range = enumerator.target_range
          if @context.order == :descending
            each_method = :reverse_each
          else
            each_method = :each
          end
          if @context.need_look_ahead?
            executors = []
            previous_executor = nil
            enumerator.send(each_method) do |shard, shard_range|
              @context.push
              current_executor = ShardExecutor.new(@context, shard, shard_range)
              if previous_executor
                previous_executor.next_executor = current_executor
                current_executor.previous_executor = previous_executor
                executors << previous_executor
              end
              previous_executor = current_executor
            end
            executors << previous_executor if previous_executor
            return if executors.empty?
            executors.each_with_index do |executor, i|
              yield(executor)
              # Keep the previous data for window function
              @context.shift if i > 0
            end
            @context.shift
          else
            enumerator.send(each_method) do |shard, shard_range|
              @context.push
              yield(ShardExecutor.new(@context, shard, shard_range))
              @context.shift
            end
          end
        end
      end

      class Window
        include Comparable
        include Loggable

        attr_reader :unit
        attr_reader :step
        def initialize(context, shard, shard_range, unit, step)
          @context = context
          @shard = shard
          @shard_range = shard_range
          @unit = unit
          @step = step

          @between = Groonga::Context.instance["between"]

          @target_range = @context.enumerator.target_range

          case @unit
          when :day
            @step_second = (60 * 60 * 24) * @step
          when :hour
            @step_second = (60 * 60) * @step
          else
            raise InvalidArgument "Unexpected unit: #{@unit.inspect}"
          end
        end

        def <=>(other)
          case @unit
          when other.unit
            @step <=> other.unit
          when :day # @unit == :day && other.unit == :hour
            1
          else # @unit == :hour && other.unit == :day
            -1
          end
        end

        def each(table)
          min = create_min_edge
          max = create_max_edge(min)
          shard_key = table.find_column(@context.enumerator.shard_key_name)
          begin
            current_min = min
            while current_min < max do
              next_min = compute_next_min_edge(current_min)
              if next_min > max
                current_max = max
              else
                current_max = WindowEdge.new(next_min.year,
                                             next_min.month,
                                             next_min.day,
                                             next_min.hour,
                                             next_min.minute,
                                             next_min.second,
                                             next_min.microsecond,
                                             :exclude)
              end
              windowed_table = select_by_range(table,
                                               shard_key,
                                               current_min,
                                               current_max)
              if windowed_table.empty?
                windowed_table.close
              else
                message = "[logical_range_filter][window] "
                message << "<#{@shard.table_name}>: "
                message << inspect_range(current_min, current_max)
                logger.log(Logger::Level::DEBUG,
                           __FILE__,
                           __LINE__,
                           __method__.to_s,
                           message)
                yield(windowed_table)
              end
              current_min = next_min
            end
          ensure
            shard_key.close
          end
        end

        private
        def create_min_edge
          min = @target_range.min
          if min
            year = min.year
            month = min.month
            day = min.day
            hour = min.hour
            minute = min.min
            second = min.sec
            microsecond = min.usec
            border = @target_range.min_border
          else
            year = @shard_range.year
            month = @shard_range.month
            day = 1
            hour = 0
            minute = 0
            second = 0
            microsecond = 0
            border = :include
          end
          WindowEdge.new(year, month, day, hour, minute, second, microsecond,
                         border)
        end

        def create_max_edge(min)
          max = @target_range.max
          if max
            year = max.year
            month = max.month
            day = max.day
            hour = max.hour
            minute = max.min
            second = max.sec
            microsecond = max.usec
            border = @target_range.max_border
          else
            next_shard_edge = @shard_range.least_over_time
            year = next_shard_edge.year
            month = next_shard_edge.month
            day = next_shard_edge.day
            hour = 0
            minute = 0
            second = 0
            microsecond = 0
            border = :exclude
          end
          WindowEdge.new(year, month, day, hour, minute, second, microsecond,
                         border)
        end

        def compute_next_min_edge(current_min)
          next_edge = Time.at(current_min.to_time.to_i + @step_second)
          year = next_edge.year
          month = next_edge.month
          day = next_edge.day
          if @unit == :day
            hour = 0
          else
            hour = next_edge.hour
          end
          minute = 0
          second = 0
          microsecond = 0
          border = :include
          WindowEdge.new(year, month, day, hour, minute, second, microsecond,
                         border)
        end

        def select_by_range(table, shard_key, min, max)
          expression = Expression.create(table)
          begin
            expression.append_object(@between, Operator::PUSH, 1)
            expression.append_object(shard_key, Operator::PUSH, 1)
            expression.append_operator(Operator::GET_VALUE, 1)
            expression.append_constant(min.to_s, Operator::PUSH, 1)
            expression.append_constant(min.border, Operator::PUSH, 1)
            expression.append_constant(max.to_s, Operator::PUSH, 1)
            expression.append_constant(max.border, Operator::PUSH, 1)
            expression.append_operator(Operator::CALL, 5)
            table.select(expression)
          ensure
            expression.close
          end
        end

        def inspect_range(min, max)
          range = ""
          if min.border == :include
            range << "["
          else
            range << "("
          end
          range << min.to_s
          range << ","
          range << max.to_s
          if max.border == :include
            range << "]"
          else
            range << ")"
          end
          range
        end
      end

      class WindowEdge
        include Comparable

        attr_reader :year
        attr_reader :month
        attr_reader :day
        attr_reader :hour
        attr_reader :minute
        attr_reader :second
        attr_reader :microsecond
        attr_reader :border
        def initialize(year, month, day, hour, minute, second, microsecond,
                       border)
          @year = year
          @month = month
          @day = day
          @hour = hour
          @minute = minute
          @second = second
          @microsecond = microsecond
          @border = border
        end

        def to_s
          format = "%04d/%02d/%02d %02d:%02d:%02d"
          format_values = [@year, @month, @day, @hour, @minute, @second]
          unless @microsecond.zero?
            format << ".%06d"
            format_values << @microsecond
          end
          format % format_values
        end

        def to_time
          Time.local(@year, @month, @day, @hour, @minute, @second, @microsecond)
        end

        def values
          [@year, @month, @day, @hour, @minute, @second, @microsecond]
        end

        def <=>(other)
          (values + [@border == :include ? 1 : 0]) <=>
            (other.values + [other.border == :include ? 1 : 0])
        end
      end

      class ShardExecutor
        include Loggable
        include QueryLoggable

        attr_reader :shard
        attr_writer :previous_executor
        attr_writer :next_executor
        def initialize(context, shard, shard_range)
          @context = context
          @shard = shard
          @shard_range = shard_range

          @shard_key = shard.key
          @target_table = @shard.table

          @filter = @context.filter
          @post_filter = @context.post_filter
          @result_sets = @context.result_sets
          @filtered_result_sets = []
          @temporary_tables = @context.temporary_tables

          @target_range = @context.enumerator.target_range

          @cover_type = @target_range.cover_type(@shard_range)

          @previous_executor = nil
          @next_executor = nil

          @initital_table = nil

          @range_index = nil
          @window = nil

          @prepared = false
          @filtered = false
        end

        def execute
          ensure_filtered

          return if @filtered_result_sets.empty?

          if @window
            @filtered_result_sets.each do |result_set|
              @window.each(result_set) do |windowed_result_set|
                @temporary_tables << windowed_result_set
                if @context.dynamic_columns.have_filtered?
                  apply_targets = [[windowed_result_set]]
                  @context.dynamic_columns.apply_filtered(apply_targets)
                end
                sort_result_set(windowed_result_set)
                return if @context.current_limit.zero?
              end
            end
          else
            apply_filtered_dynamic_columns
            @filtered_result_sets.each do |result_set|
              sort_result_set(result_set)
            end
          end
        end

        def add_initial_stage_context(apply_targets)
          ensure_prepared
          return unless @initial_table
          apply_targets << [@initial_table, {context: true}]
        end

        def add_filtered_stage_context(apply_targets)
          ensure_filtered
          @filtered_result_sets.each do |table|
            apply_targets << [table, {context: true}]
          end
        end

        private
        def have_record?
          return false if @cover_type == :none
          return false if @target_table.empty?
          true
        end

        def ensure_prepared
          return if @prepared
          @prepared = true

          return unless have_record?

          if @shard_key.nil?
            message = "[logical_range_filter] shard_key doesn't exist: " +
                      "<#{@shard.key_name}>"
            raise InvalidArgument, message
          end

          @expression_builder = RangeExpressionBuilder.new(@shard_key,
                                                           @target_range)
          @expression_builder.filter = @filter
          index_info = @shard_key.find_index(Operator::LESS)
          if index_info
            index = index_info.index
            @context.referred_objects << index
            @range_index = index if use_range_index?
          end

          if @context.dynamic_columns.have_initial?
            if @cover_type == :all
              @target_table = @target_table.select_all
            else
              @expression_builder.filter = nil
              @target_table = create_expression(@target_table) do |expression|
                @expression_builder.build(expression, @shard_range)
                @target_table.select(expression)
              end
              @expression_builder.filter = @filter
              @cover_type = :all
            end
            @temporary_tables << @target_table
            @initial_table = @target_table
          end

          @window = detect_window
        end

        def compute_limit(n_source_records)
          current_limit = @context.current_limit
          if current_limit < 0
            n_source_records
          else
            current_limit
          end
        end

        def decide_use_range_index(use, reason, line, method)
          message = "[logical_range_filter]"
          if use
            message << "[range-index] "
          else
            message << "[select] "
          end
          message << "<#{@shard.table_name}>: "
          message << reason
          logger.log(Logger::Level::DEBUG,
                     __FILE__,
                     line,
                     method.to_s,
                     message)
          use
        end

        def use_range_index?
          use_range_index_parameter_message =
            "force by use_range_index parameter"
          case @context.use_range_index
          when true
            return decide_use_range_index(true,
                                          use_range_index_parameter_message,
                                          __LINE__, __method__)
          when false
            return decide_use_range_index(false,
                                          use_range_index_parameter_message,
                                          __LINE__, __method__)
          end

          range_index_logical_parameter_message =
            "force by range_index logical parameter"
          case Parameters.range_index
          when :always
            return decide_use_range_index(true,
                                          range_index_logical_parameter_message,
                                          __LINE__, __method__)
          when :never
            return decide_use_range_index(false,
                                          range_index_logical_parameter_message,
                                          __LINE__, __method__)
          end

          unless @context.dynamic_columns.empty?
            reason = "dynamic columns are used"
            return decide_use_range_index(false, reason, __LINE__, __method__)
          end

          unless @context.post_filter.nil?
            reason = "post_filter is used"
            return decide_use_range_index(false, reason, __LINE__, __method__)
          end

          unless @context.sort_keys.empty?
            reason = "sort_keys is used"
            return decide_use_range_index(false, reason, __LINE__, __method__)
          end

          current_limit = @context.current_limit
          if current_limit < 0
            reason = "limit is negative: <#{current_limit}>"
            return decide_use_range_index(false, reason, __LINE__, __method__)
          end

          required_n_records = @context.current_offset + current_limit
          max_n_records = @shard.table.size
          is_large_shard = (max_n_records > @context.large_shard_threshold)
          if is_large_shard and max_n_records <= required_n_records
            reason = "the number of required records (#{required_n_records}) "
            reason << ">= "
            reason << "the number of records in shard (#{max_n_records})"
            return decide_use_range_index(false, reason,
                                          __LINE__, __method__)
          end

          threshold = @context.threshold
          if threshold <= 0.0
            reason = "threshold is negative: <#{threshold}>"
            return decide_use_range_index(true, reason,
                                          __LINE__, __method__)
          end
          if threshold >= 1.0
            reason = "threshold (#{threshold}) >= 1.0"
            return decide_use_range_index(false, reason,
                                          __LINE__, __method__)
          end

          table = @shard.table
          estimated_n_records = 0
          case @cover_type
          when :all
            if @filter
              create_expression(table) do |expression|
                @expression_builder.build_all(expression)
                unless range_index_available_expression?(expression,
                                                         __LINE__, __method__)
                  return false
                end
                estimated_n_records = expression.estimate_size(table)
              end
            else
              estimated_n_records = max_n_records
            end
          when :partial_min
            create_expression(table) do |expression|
              @expression_builder.build_partial_min(expression)
              unless range_index_available_expression?(expression,
                                                       __LINE__, __method__)
                return false
              end
              estimated_n_records = expression.estimate_size(table)
            end
          when :partial_max
            create_expression(table) do |expression|
              @expression_builder.build_partial_max(expression)
              unless range_index_available_expression?(expression,
                                                       __LINE__, __method__)
                return false
              end
              estimated_n_records = expression.estimate_size(table)
            end
          when :partial_min_and_max
            create_expression(table) do |expression|
              @expression_builder.build_partial_min_and_max(expression)
              unless range_index_available_expression?(expression,
                                                       __LINE__, __method__)
                return false
              end
              estimated_n_records = expression.estimate_size(table)
            end
          end

          if is_large_shard and estimated_n_records <= required_n_records
            reason = "the number of required records (#{required_n_records}) "
            reason << ">= "
            reason << "the number of estimated records (#{estimated_n_records})"
            return decide_use_range_index(false, reason,
                                          __LINE__, __method__)
          end

          max_n_unmatched_records =
              compute_max_n_unmatched_records(max_n_records,
                                              required_n_records)
          if estimated_n_records >= max_n_unmatched_records
            reason = "the max number of unmatched records (#{max_n_unmatched_records}) "
            reason << "<= "
            reason << "the number of estimated records (#{estimated_n_records})"
            return decide_use_range_index(true, reason,
                                          __LINE__, __method__)
          end

          hit_ratio = estimated_n_records / max_n_records.to_f
          use_range_index_by_hit_ratio = (hit_ratio >= threshold)
          if use_range_index_by_hit_ratio
            relation = ">="
          else
            relation = "<"
          end
          reason = "hit ratio "
          reason << "(#{hit_ratio}=#{estimated_n_records}/#{max_n_records}) "
          reason << "#{relation} threshold (#{threshold})"
          decide_use_range_index(use_range_index_by_hit_ratio, reason,
                                 __LINE__, __method__)
        end

        def range_index_available_expression?(expression, line, method_name)
          nested_reference_vector_column_accessor =
            find_nested_reference_vector_column_accessor(expression)
          if nested_reference_vector_column_accessor
            reason = "nested reference vector column accessor can't be used: "
            reason << "<#{nested_reference_vector_column_accessor.name}>"
            return decide_use_range_index(false, reason, line, method_name)
          end

          selector_only_procedure = find_selector_only_procedure(expression)
          if selector_only_procedure
            reason = "selector only procedure can't be used: "
            reason << "<#{selector_only_procedure.name}>"
            return decide_use_range_index(false, reason, line, method_name)
          end

          true
        end

        def find_nested_reference_vector_column_accessor(expression)
          expression.codes.each do |code|
            value = code.value
            next unless value.is_a?(Accessor)

            sub_accessor = value
            while sub_accessor.have_next?
              object = sub_accessor.object
              return value if object.is_a?(Column) and object.vector?
              sub_accessor = sub_accessor.next
            end
          end
          nil
        end

        def find_selector_only_procedure(expression)
          expression.codes.each do |code|
            value = code.value
            return value if value.is_a?(Procedure) and value.selector_only?
          end
          nil
        end

        def ensure_filtered
          return if @filtered

          @filtered = true

          ensure_prepared
          return unless have_record?

          if @context.dynamic_columns.have_initial?
            apply_targets = []
            if @previous_executor
              @previous_executor.add_initial_stage_context(apply_targets)
            end
            apply_targets << [@target_table]
            if @next_executor
              @next_executor.add_initial_stage_context(apply_targets)
            end
            @context.dynamic_columns.apply_initial(apply_targets)
          end

          execute_filter(@range_index)

          return unless @context.need_look_ahead?

          apply_targets = []
          @filtered_result_sets = @filtered_result_sets.collect do |result_set|
            if result_set == @shard.table
              result_set = result_set.select_all
              @temporary_tables << result_set
            end
            apply_targets << [result_set]
            result_set
          end
          @context.dynamic_columns.apply_filtered(apply_targets,
                                                  window_function: false)
        end

        def execute_filter(range_index)
          case @cover_type
          when :all
            filter_shard_all(range_index)
          when :partial_min
            if range_index
              filter_by_range(range_index,
                              @target_range.min, @target_range.min_border,
                              nil, nil)
            else
              filter_table do |expression|
                @expression_builder.build_partial_min(expression)
              end
            end
          when :partial_max
            if range_index
              filter_by_range(range_index,
                              nil, nil,
                              @target_range.max, @target_range.max_border)
            else
              filter_table do |expression|
                @expression_builder.build_partial_max(expression)
              end
            end
          when :partial_min_and_max
            if range_index
              filter_by_range(range_index,
                              @target_range.min, @target_range.min_border,
                              @target_range.max, @target_range.max_border)
            else
              filter_table do |expression|
                @expression_builder.build_partial_min_and_max(expression)
              end
            end
          end
        end

        def filter_shard_all(range_index)
          table = @target_table
          if @filter.nil?
            if @post_filter.nil? and table.size <= @context.current_offset
              @context.current_offset -= table.size
              return
            end
            if range_index
              filter_by_range(range_index,
                              nil, nil,
                              nil, nil)
            else
              add_filtered_result_set(table)
            end
          else
            if range_index
              filter_by_range(range_index,
                              nil, nil,
                              nil, nil)
            else
              filter_table do |expression|
                @expression_builder.build_all(expression)
              end
            end
          end
        end

        def create_expression(table)
          expression = Expression.create(table)
          begin
            yield(expression)
          ensure
            expression.close
          end
        end

        def filter_by_range(range_index, min, min_border, max, max_border)
          lexicon = range_index.domain
          @context.referred_objects << lexicon
          data_table = range_index.range
          @context.referred_objects << data_table
          flags = build_range_search_flags(min_border, max_border)

          result_set = HashTable.create(:flags => ObjectFlags::WITH_SUBREC,
                                        :key_type => data_table)
          @temporary_tables << result_set
          n_matched_records = 0
          TableCursor.open(lexicon,
                           :min => min,
                           :max => max,
                           :flags => flags) do |table_cursor|
            options = {
              :offset => @context.current_offset,
              :limit => compute_limit(data_table.size),
            }
            max_n_unmatched_records =
              compute_max_n_unmatched_records(data_table.size,
                                              options[:limit])
            options[:max_n_unmatched_records] = max_n_unmatched_records
            if @filter
              create_expression(data_table) do |expression|
                expression.parse(@filter)
                options[:expression] = expression
                IndexCursor.open(table_cursor, range_index) do |index_cursor|
                  n_matched_records = index_cursor.select(result_set, options)
                end
              end
              # TODO: Add range information
              query_logger.log(:size, ":",
                               "filter(#{n_matched_records})" +
                               "[#{@shard.table_name}]: #{@filter}")
            else
              IndexCursor.open(table_cursor, range_index) do |index_cursor|
                n_matched_records = index_cursor.select(result_set, options)
              end
              # TODO: Add range information
              query_logger.log(:size, ":",
                               "filter(#{n_matched_records})" +
                               "[#{@shard.table_name}]")
            end
            if n_matched_records == -1
              fallback_message =
                "fallback because there are too much unmatched records: "
              fallback_message << "<#{max_n_unmatched_records}>"
              decide_use_range_index(false,
                                     fallback_message,
                                     __LINE__, __method__)
              execute_filter(nil)
              return
            end
          end

          if n_matched_records <= @context.current_offset
            @context.current_offset -= n_matched_records
            return
          end

          if @context.current_offset > 0
            @context.current_offset = 0
          end
          if @context.current_limit > 0
            @context.current_limit -= result_set.size
          end
          @result_sets << result_set
        end

        def build_range_search_flags(min_border, max_border)
          flags = TableCursorFlags::BY_KEY
          case @context.order
          when :ascending
            flags |= TableCursorFlags::ASCENDING
          when :descending
            flags |= TableCursorFlags::DESCENDING
          end
          case min_border
          when :include
            flags |= TableCursorFlags::GE
          when :exclude
            flags |= TableCursorFlags::GT
          end
          case max_border
          when :include
            flags |= TableCursorFlags::LE
          when :exclude
            flags |= TableCursorFlags::LT
          end
          flags
        end

        def compute_max_n_unmatched_records(max_n_records, limit)
          return limit if max_n_records.zero?

          scale = Math.log(max_n_records) ** 2
          max_n_unmatched_records = limit * scale
          max_n_sample_records = max_n_records
          if max_n_sample_records > 10000
            max_n_sample_records = max_n_sample_records / scale
          end
          if max_n_unmatched_records > max_n_sample_records
            max_n_unmatched_records = max_n_sample_records
          end
          max_n_unmatched_records.ceil
        end

        def filter_table
          table = @target_table
          create_expression(table) do |expression|
            yield(expression)
            result_set = table.select(expression)
            @temporary_tables << result_set
            add_filtered_result_set(result_set)
          end
        end

        def apply_post_filter(table)
          create_expression(table) do |expression|
            expression.parse(@post_filter)
            table.select(expression)
          end
        end

        def add_filtered_result_set(result_set)
          return if result_set.empty?
          @filtered_result_sets << result_set
        end

        def apply_filtered_dynamic_columns
          return unless @context.dynamic_columns.have_filtered?

          apply_targets = []
          if @previous_executor
            @previous_executor.add_filtered_stage_context(apply_targets)
          end
          @filtered_result_sets = @filtered_result_sets.collect do |result_set|
            if result_set == @shard.table
              result_set = result_set.select_all
              @temporary_tables << result_set
            end
            apply_targets << [result_set]
            result_set
          end
          if @next_executor
            @next_executor.add_filtered_stage_context(apply_targets)
          end
          options = {}
          if @context.need_look_ahead?
            options[:normal] = false
          end
          @context.dynamic_columns.apply_filtered(apply_targets, options)
        end

        def sort_result_set(result_set)
          unless @post_filter.nil?
            result_set = apply_post_filter(result_set)
            @temporary_tables << result_set
          end

          if result_set.size <= @context.current_offset
            @context.current_offset -= result_set.size
            return
          end

          if @context.sort_keys.empty?
            sort_keys = [
              {
                :key => @context.enumerator.shard_key_name,
                :order => @context.order,
              },
            ]
          else
            sort_keys = @context.sort_keys.collect do |sort_key|
              if sort_key.start_with?("-")
                key = sort_key[1..-1]
                order = :descending
              else
                key = sort_key
                order = :ascending
              end
              {:key => key, :order => order}
            end
          end
          if @context.current_limit > 0
            limit = @context.current_limit
          else
            limit = result_set.size
          end
          sorted_result_set = result_set.sort(sort_keys,
                                              :offset => @context.current_offset,
                                              :limit => limit)
          @temporary_tables << sorted_result_set
          @result_sets << sorted_result_set
          if @context.current_offset > 0
            @context.current_offset = 0
          end
          if @context.current_limit > 0
            @context.current_limit -= sorted_result_set.size
          end
          unparsed_sort_keys = sort_keys.collect do |sort_key|
            key = sort_key[:key]
            key = "-#{key}" if sort_key[:order] == :descending
            key
          end
          query_logger.log(:size,
                           ":",
                           "sort(#{sorted_result_set.size})" +
                           "[#{@shard.table_name}]: " +
                           unparsed_sort_keys.join(","))
        end

        def detect_window
          # TODO: return nil if result_set is small enough
          windows = []
          @context.time_classify_types.each do |type|
            case type
            when "minute", "second"
              windows << Window.new(@context, @shard, @shard_range, :hour, 1)
            when "day", "hour"
              unless @shard_range.is_a?(LogicalEnumerator::DayShardRange)
                windows << Window.new(@context, @shard, @shard_range, :day, 1)
              end
            end
          end
          windows.max
        end
      end
    end
  end
end
