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
                  "post_filter",
                ])

        def run_body(input)
          context = ShardCountContext.new(input)
                    total = 0
          begin
            n_elements = 0
            is_first = true
            counter = Counter.new(context)
            count_result = counter.count
          ensure
            context.close
          end
          query_logger.log(:size, ":", "count(#{count_result})")
          writer.write(count_result)
        end

        private
        def cache_key(input)
          key = "logical_count\0"
          key << "#{input[:logical_table]}\0"
          key << "#{input[:shard_key]}\0"
          key << "#{input[:min]}\0"
          key << "#{input[:min_border]}\0"
          key << "#{input[:max]}\0"
          key << "#{input[:max_border]}\0"
          key << "#{input[:filter]}\0"
          key << "#{input[:post_filter]}\0"
          dynamic_columns = DynamicColumns.parse("[logical_count]", input)
          key << dynamic_columns.cache_key
          key
        end
       
        class ShardCountContext
          attr_reader :enumerator
          attr_reader :filter
          attr_reader :post_filter
          attr_reader :dynamic_columns
          attr_reader :time_classify_types
          attr_reader :result_sets
          def initialize(input)
            @input = input
            @enumerator = LogicalEnumerator.new("logical_count",
                                                @input,
                                                { :unref_immediately => true })
            @filter = @input[:filter]
            @post_filter = @input[:post_filter]

            @dynamic_columns = DynamicColumns.parse("[logical_count]",
                                                    @input)
            @result_sets = []
            @temporary_tables_queue = []
            @time_classify_types = detect_time_classify_types
            @referred_objects_queue = []
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
            @result_sets.clear
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

        class Counter
          def initialize(context)
            @context = context
          end

          def count
            have_shard = false
            have_result_set = false
            total = 0
            each_shard_executor do |shard_executor|
              have_shard = true
              total += shard_executor.count
            end
            unless have_shard
              enumerator = @context.enumerator
              message =
                "[logical_count] no shard exists: " +
                "logical_table: <#{enumerator.logical_table}>: " +
                "shard_key: <#{enumerator.shard_key_name}>"
              raise InvalidArgument, message
            end
            total
          end

          private
          def each_shard_executor(&block)
            enumerator = @context.enumerator

            if @context.need_look_ahead?
              previous_executor = nil
              enumerator.send(:each) do |shard, shard_range|
                @context.push
                current_executor = ShardCounter.new(@context, shard, shard_range)
                if previous_executor
                  previous_executor.next_executor = current_executor
                  current_executor.previous_executor = previous_executor
                  yield(previous_executor)
                  @context.shift unless previous_executor.shard.first?
                end
                if shard.last?
                  yield(current_executor)
                  @context.shift
                  break
                end
                previous_executor = current_executor
              end
            else
              enumerator.send(:each) do |shard, shard_range|
                @context.push
                yield(ShardCounter.new(@context, shard, shard_range))
                @context.shift
              end
            end
          end
        end

        class ShardCounter
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

          def count
            ensure_filtered

            if @range_index
              return count_n_records_in_range
            end
            
            return 0 if @filtered_result_sets.empty?
            total = 0
            if @window
              @filtered_result_sets.each do |result_set|
                @window.each(result_set) do |windowed_result_set|
                  @temporary_tables << windowed_result_set
                  if @context.dynamic_columns.have_filtered?
                    apply_targets = [[windowed_result_set]]
                    @context.dynamic_columns.apply_filtered(apply_targets)
                  end
                  unless @post_filter.nil?
                    windowed_result_set = apply_post_filter(windowed_result_set)
                    @temporary_tables << windowed_result_set
                  end
                  total += windowed_result_set.size
                end
              end
            else
              apply_filtered_dynamic_columns
              @filtered_result_sets.each do |result_set|
                unless @post_filter.nil?
                  result_set = apply_post_filter(result_set)
                  @temporary_tables << result_set
                end
                total += result_set.size
              end
            end
            return total
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
            #return false if @target_table.empty?
            true
          end

          def ensure_prepared
            return if @prepared
            @prepared = true
            return unless have_record?
            if @shard_key.nil?
              message = "[logical_count] shard_key doesn't exist: " +
                        "<#{@shard.key_name}>"
              raise InvalidArgument, message
            end
            @expression_builder = RangeExpressionBuilder.new(@shard_key,
                                                            @target_range)
            @expression_builder.filter = @filter
            table_name = shard.table_name
            if @filter or @post_filter
              log_use_range_index(false, table_name, "need filter",
                                  __LINE__, __method__)
            elsif @cover_type == :all
              log_use_range_index(false, table_name, "covered",
                                  __LINE__, __method__)
            else
              index_info = @shard_key.find_index(Operator::LESS)
              @range_index = index_info.index if index_info
              if @range_index
                @context.referred_objects << @range_index
                log_use_range_index(true, table_name, "range index is available",
                  __LINE__, __method__)
              else
                log_use_range_index(false, table_name, "no range index",
                  __LINE__, __method__)
              end
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

          private
          def log_use_range_index(use, table_name, reason, line, method)
            message = "[logical_count]"
            if use
              message << "[range-index]"
            else
              message << "[select]"
            end
            message << " <#{table_name}>: #{reason}"
            logger.log(Logger::Level::DEBUG,
                       __FILE__,
                       line,
                       method.to_s,
                       message)
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

            execute_filter
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

          def execute_filter
            return if @range_index
            if @cover_type == :all and @filter.nil?
              if @post_filter and @context.dynamic_columns.have_filtered?
                filtered_table = @target_table.select_all
                @temporary_tables << filtered_table
                @filtered_result_sets << filtered_table
              else
                @filtered_result_sets << @target_table
              end
            else
              expression = Expression.create(@target_table)
              begin
                expression_builder = RangeExpressionBuilder.new(@shard_key,
                                                                @target_range)
                expression_builder.filter = @filter
                case @cover_type
                when :all
                  expression_builder.build_all(expression)
                when :partial_min
                  expression_builder.build_partial_min(expression)
                when :partial_max
                  expression_builder.build_partial_max(expression)
                when :partial_min_and_max
                  expression_builder.build_partial_min_and_max(expression)
                end
                filtered_table = @target_table.select(expression)
              ensure
                expression.close
              end
              @temporary_tables << filtered_table
              @filtered_result_sets << filtered_table
            end
          end

          def count_n_records_in_range
            case @cover_type
            when :partial_min
              min = @target_range.min
              min_border = @target_range.min_border
              max = nil
              max_bordre = nil
            when :partial_max
              min = nil
              min_bordre = nil
              max = @target_range.max
              max_border = @target_range.max_border
            when :partial_min_and_max
              min = @target_range.min
              min_border = @target_range.min_border
              max = @target_range.max
              max_border = @target_range.max_border
            end

            flags = TableCursorFlags::BY_KEY
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

            lexicon = @range_index.table
            @context.referred_objects << lexicon
            TableCursor.open(lexicon,
                            :min => min,
                            :max => max,
                            :flags => flags) do |table_cursor|
              IndexCursor.open(table_cursor, @range_index) do |index_cursor|
                index_cursor.count
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

          def apply_post_filter(table)
            create_expression(table) do |expression|
              expression.parse(@post_filter)
              table.select(expression)
            end
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
