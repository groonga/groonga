module Groonga
  module Sharding
    class Window
      include Comparable
      include Loggable
      
      attr_reader :unit
      attr_reader :step
      def initialize(context, shard, shard_range, unit, step, tag)
        @context = context
        @shard = shard
        @shard_range = shard_range
        @unit = unit
        @step = step
        @tag = tag
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
          while current_min < max
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
              message = "#{@tag}[window] "
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
  end
end
