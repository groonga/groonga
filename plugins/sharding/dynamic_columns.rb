module Groonga
  module Sharding
    class DynamicColumns
      class << self
        def parse(input)
          initial_contexts = {}
          filtered_contexts = {}
          output_contexts = {}
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
            contexts[label] = DynamicColumnExecuteContext.new(label, arguments)
          end

          new(DynamicColumnExecuteContexts.new(initial_contexts),
              DynamicColumnExecuteContexts.new(filtered_contexts),
              DynamicColumnExecuteContexts.new(output_contexts))
        end
      end

      def initialize(initial_contexts,
                     filtered_contexts,
                     output_contexts)
        @initial_contexts = initial_contexts
        @filtered_contexts = filtered_contexts
        @output_contexts = output_contexts
      end

      def have_initial?
        not @initial_contexts.empty?
      end

      def have_filtered?
        not @filtered_contexts.empty?
      end

      def have_output?
        not @output_contexts.empty?
      end

      def each_initial(&block)
        @initial_contexts.tsort_each(&block)
      end

      def each_filtered(&block)
        @filtered_contexts.tsort_each(&block)
      end

      def each_output(&block)
        @output_contexts.tsort_each(&block)
      end

      def each(&block)
        each_initial(&block)
        each_filtered(&block)
        each_output(&block)
      end

      def apply_initial(targets)
        apply(@initial_contexts, targets)
      end

      def apply_filtered(targets)
        apply(@filtered_contexts, targets)
      end

      def apply_output(targets)
        apply(@output_contexts, targets)
      end

      def empty?
        @initial_contexts.empty? and
          @filtered_contexts.empty? and
          @output_contexts.empty?
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

      private
      def apply(contexts, targets)
        window_function_contexts = []
        normal_contexts = []
        contexts.each do |context|
          if context.window_function?
            window_function_contexts << context
          else
            normal_contexts << context
          end
        end

        targets.each do |result_set, options|
          normal_contexts.each do |context|
            context.apply(result_set, options)
          end
        end
        window_function_contexts.each do |context|
          context.apply_window_function(targets)
        end
      end
    end

    class DynamicColumnExecuteContexts
      include Enumerable
      include TSort

      def initialize(contexts)
        @contexts = contexts
        @dependencies = {}
        @contexts.each do |label, context|
          dependencies = []
          # TODO: Too rough.
          context.value.split(/[ \(\),]+/).each do |component|
            depended_context = @contexts[component]
            dependencies << depended_context if depended_context
          end
          context.window_sort_keys.each do |key|
            depended_context = @contexts[key.gsub(/\A-/, "")]
            dependencies << depended_context if depended_context
          end
          context.window_group_keys.each do |key|
            depended_context = @contexts[key]
            dependencies << depended_context if depended_context
          end
          @dependencies[label] = dependencies
        end
      end

      def empty?
        @contexts.empty?
      end

      def each(&block)
        @contexts.each_value(&block)
      end

      def tsort_each_node(&block)
        @contexts.each_value(&block)
      end

      def tsort_each_child(context, &block)
        @dependencies[context.label].each(&block)
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

      def window_function?
        (not @window_sort_keys.empty?) or
          (not @window_group_keys.empty?)
      end

      def apply(table, options=nil)
        pp [table, options, context_target?(options)]
        if context_target?(options)
          column = table.find_column(@label)
          p column if column.is_a?(Accessor)
        else
          column = table.create_column(@label, @flags, @type)
        end
        p column
        return if table.empty?

        condition = nil
        condition = options[:condition] if options
        expression = Expression.create(table)
        begin
          expression.parse(@value)
          expression.condition = condition if condition
          table.apply_expression(column, expression)
        ensure
          expression.close
        end
      end

      def apply_window_function(targets)
        executor = WindowFunctionExecutor.new
        begin
          executor.source = @value
          executor.sort_keys = @window_sort_keys.join(", ")
          executor.group_keys = @window_group_keys.join(", ")
          executor.output_column_name = @label
          targets.each do |table, options|
            unless context_target?(options)
              table.create_column(@label, @flags, @type)
            end
            next if table.empty?
            executor.add_table(table)
          end
          executor.execute
        ensure
          executor.close
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

      def context_target?(options)
        options and options[:context]
      end

      def error_message_tag
        "[logical_select][columns][#{@stage}][#{@label}]"
      end
    end
  end
end
