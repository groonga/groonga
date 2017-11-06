module Groonga
  module Sharding
    class DynamicColumns
      class << self
        def parse(input)
          initial_contexts = []
          filtered_contexts = []
          output_contexts = []
          labeled_arguments = LabeledArguments.new(input, /columns?/)
          labeled_arguments.each do |label, arguments|
            contexts = nil
            case arguments["stage"]
            when "initial"
              contexts = initial_contexts
            when "filtered"
              contexts = filtered_contexts
            when "output"
              contexts = output_contexts
            else
              next
            end
            contexts << DynamicColumnExecuteContext.new(label, arguments)
          end

          new(initial_contexts,
              filtered_contexts,
              output_contexts)
        end
      end

      def initialize(initial_contexts,
                     filtered_contexts,
                     output_contexts)
        @initial_contexts = initial_contexts
        @filtered_contexts = filtered_contexts
        @output_contexts = output_contexts
      end

      def each_initial(&block)
        @initial_contexts.each(&block)
      end

      def each_filtered(&block)
        @filtered_contexts.each(&block)
      end

      def each_output(&block)
        @output_contexts.each(&block)
      end

      def each(&block)
        each_initial(&block)
        each_filtered(&block)
        each_output(&block)
      end

      def empty?
        @initial_contexts.empty? and
          @filtered_contexts.empty? and
          @output_contexts.empty?
      end

      def close
        each do |context|
          context.close
        end
      end

      def cache_key
        key = ""
        [
          @initial_contexts,
          @filtered_contexts,
          @output_contexts,
        ].each do |contexts|
          contexts.sort_by(&:label).each do |context|
            key << "#{context.label}\0"
            key << "#{context.stage}\0"
            key << "#{context.type.name}\0"
            key << "#{context.flags}\0"
            key << "#{context.value}\0"
            key << "#{context.window_sort_keys.join(',')}\0"
            key << "#{context.window_group_keys.join(',')}\0"
          end
        end
        key
      end
    end

    class DynamicColumnExecuteContext
      include KeysParsable

      attr_reader :label
      attr_reader :stage
      attr_reader :type
      attr_reader :flags
      attr_reader :value
      attr_reader :window_sort_keys
      attr_reader :window_group_keys
      def initialize(label, arguments)
        @label = label
        @stage = arguments["stage"]
        @type = parse_type(arguments["type"])
        @flags = parse_flags(arguments["flags"] || "COLUMN_SCALAR")
        @value = arguments["value"]
        @window_sort_keys = parse_keys(arguments["window.sort_keys"])
        @window_group_keys = parse_keys(arguments["window.group_keys"])
      end

      def close
      end

      def apply(table, condition=nil)
        column = table.create_column(@label, @flags, @type)
        return if table.empty?

        expression = Expression.create(table)
        begin
          expression.parse(@value)
          if @window_sort_keys.empty? and @window_group_keys.empty?
            expression.condition = condition if condition
            table.apply_expression(column, expression)
          else
            table.apply_window_function(column, expression,
                                        :sort_keys => @window_sort_keys,
                                        :group_keys => @window_group_keys)
          end
        ensure
          expression.close
        end
      end

      private
      def parse_type(type_raw)
        return nil if type_raw.nil?

        type = Context.instance[type_raw]
        if type.nil?
          message = "#{error_message_tag} unknown type: <#{type_raw}>"
          raise InvalidArgument, message
        end

        case type
        when Type, Table
          type
        else
          message = "#{error_message_tag} invalid type: #{type.grn_inspect}"
          raise InvalidArgument, message
        end
      end

      def parse_flags(flags_raw)
        Column.parse_flags(error_message_tag, flags_raw)
      end

      def error_message_tag
        "[logical_select][columns][#{@stage}][#{@label}]"
      end
    end
  end
end
