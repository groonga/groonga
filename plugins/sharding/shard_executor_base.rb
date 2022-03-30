module Groonga
  module Sharding
    class StreamShardExecutor
      # Derived class must imprement methods below.
      # - execute_filter(context)
      # - use_range_index?

      attr_reader :shard
      attr_writer :previous_executor
      attr_writer :next_executor
      def initialize(command_name, context, shard, shard_range)
        @command_name = command_name
        @context = context
        @shard = shard
        @shard_range = shard_range

        @shard_key = shard.key
        @target_table = @shard.table

        @filter = @context.filter
        @post_filter = @context.post_filter
        @filtered_result_sets = []
        @temporary_tables = @context.temporary_tables

        @target_range = @context.enumerator.target_range

        @cover_type = @target_range.cover_type(@shard_range)

        @previous_executor = nil
        @next_executor = nil

        @range_index = nil
        @window = nil

        @prepared = false
        @filtered = false
      end

      protected
      def have_record?
        return false if @cover_type == :none
        return false if @target_table.empty?
        true
      end

      def add_initial_stage_context(apply_targets)
        ensure_prepared
        return unless @initial_table
        apply_targets << [@initial_table, { context: true }]
      end

      def add_filtered_stage_context(apply_targets)
        ensure_filtered
        @filtered_result_sets.each do |table|
          apply_targets << [table, { context: true }]
        end
      end

      def ensure_prepared
        return if @prepared
        @prepared = true

        return unless have_record?

        if @shard_key.nil?
          message = "[#{@command_name}] shard_key doesn't exist: " +
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
      
      def filter_table
        table = @target_table
        create_expression(table) do |expression|
          yield(expression)
          result_set = table.select(expression)
          @temporary_tables << result_set
          add_filtered_result_set(result_set)
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

      def detect_window
        # TODO: return nil if result_set is small enough
        tag = "[#{@command_name}]"
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
