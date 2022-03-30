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
              writer.open_result_set(result_set, output_columns, -1)
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
            writer.close_result_set
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

      class ExecuteContext < StreamExecuteContext
        include KeysParsable

        attr_reader :use_range_index
        attr_reader :order
        attr_reader :offset
        attr_reader :limit
        attr_reader :sort_keys
        attr_accessor :current_offset
        attr_accessor :current_limit
        attr_reader :threshold
        attr_reader :large_shard_threshold
        attr_reader :result_sets
        def initialize(input)
          super("logical_range_filter", input)
          @use_range_index = parse_use_range_index(@input[:use_range_index])
          @order = parse_order(@input, :order)
          @offset = (@input[:offset] || 0).to_i
          @limit = (@input[:limit] || 10).to_i
          @sort_keys = parse_keys(@input[:sort_keys])

          @current_offset = @offset
          @current_limit = @limit

          @result_sets = []

          @threshold = compute_threshold
          @large_shard_threshold = compute_large_shard_threshold
        end

        def close
          super
          @result_sets.clear
        end

        def consume_result_sets
          while (result_set = @result_sets.shift)
            yield(result_set)
          end
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
      end

      class Executor
        def initialize(context)
          @context = context
        end

        def execute
          have_shard = false
          have_result_set = false
          each_shard_executor do |shard_executor|
            have_shard = true
            shard_executor.execute
            @context.consume_result_sets do |result_set|
              have_result_set = true
              yield(result_set)
            end
            break if @context.current_limit.zero?
          end
          unless have_shard
            enumerator = @context.enumerator
            message =
              "[logical_range_filter] no shard exists: " +
              "logical_table: <#{enumerator.logical_table}>: " +
              "shard_key: <#{enumerator.shard_key_name}>"
            raise InvalidArgument, message
          end
          return if have_result_set

          each_shard_executor do |shard_executor|
            shard = shard_executor.shard
            result_set = HashTable.create(:flags => ObjectFlags::WITH_SUBREC,
                                          :key_type => shard.table)
            @context.push
            @context.temporary_tables << result_set
            targets = [[result_set]]
            @context.dynamic_columns.apply_initial(targets)
            @context.dynamic_columns.apply_filtered(targets)
            yield(result_set)
            @context.shift
            break
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
            previous_executor = nil
            enumerator.send(each_method) do |shard, shard_range|
              @context.push
              current_executor = ShardExecutor.new(@context, shard, shard_range)
              if previous_executor
                previous_executor.next_executor = current_executor
                current_executor.previous_executor = previous_executor
                yield(previous_executor)
                @context.shift unless previous_executor.shard.first?
              end
              if shard.last?
                yield(current_executor)
                @context.shift
              end
              previous_executor = current_executor
            end
          else
            enumerator.send(each_method) do |shard, shard_range|
              @context.push
              yield(ShardExecutor.new(@context, shard, shard_range))
              @context.shift
            end
          end
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
          max_n_unmatched_records = limit * 2 * scale
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
          tag = "[logical_range_filter]"
          windows = []
          @context.time_classify_types.each do |type|
            case type
            when "minute", "second"
              windows << Window.new(@context, @shard, @shard_range, :hour, 1, tag)
            when "day", "hour"
              unless @shard_range.is_a?(LogicalEnumerator::DayShardRange)
                windows << Window.new(@context, @shard, @shard_range, :day, 1, tag)
              end
            end
          end
          windows.max
        end
      end
    end
  end
end
