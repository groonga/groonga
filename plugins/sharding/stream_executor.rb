module Groonga
  module Sharding
    class StreamExecutor
      def initialize(context, shard_executor_class)
        @context = context
        @shard_executor_class = shard_executor_class
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
            current_executor = @shard_executor_class.new(@context, shard, shard_range)
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
            yield(@shard_executor_class.new(@context, shard, shard_range))
            @context.shift
          end
        end
      end
    end
  end
end
