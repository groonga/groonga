module Groonga
  module Sharding
    class LogicalSelectCommand < Command
      register("logical_select",
               [
                 "logical_table",
                 "shard_key",
                 "min",
                 "min_border",
                 "max",
                 "max_border",
                 "filter",
                 # Deprecated since 6.1.5. Use sort_keys instead.
                 "sortby",
                 "output_columns",
                 "offset",
                 "limit",
                 "drilldown",
                 # Deprecated since 6.1.5. Use drilldown_sort_keys instead.
                 "drilldown_sortby",
                 "drilldown_output_columns",
                 "drilldown_offset",
                 "drilldown_limit",
                 "drilldown_calc_types",
                 "drilldown_calc_target",
                 "sort_keys",
                 "drilldown_sort_keys",
                 "match_columns",
                 "query",
                 "drilldown_filter",
                 "load_table",
                 "load_columns",
                 "load_values",
               ])

      def run_body(input)
        context = ExecuteContext.new(input)
        begin
          executor = Executor.new(context)
          executor.execute

          load_records(context)

          n_results = 1
          n_plain_drilldowns = context.plain_drilldown.n_result_sets
          n_labeled_drilldowns = context.labeled_drilldowns.n_result_sets
          if n_plain_drilldowns > 0
            n_results += n_plain_drilldowns
          elsif
            if n_labeled_drilldowns > 0
              n_results += 1
            end
          end

          writer.array("RESULT", n_results) do
            write_records(writer, context)
            if n_plain_drilldowns > 0
              write_plain_drilldowns(writer, context)
            elsif n_labeled_drilldowns > 0
              write_labeled_drilldowns(writer, context)
            end
          end
        ensure
          context.close
        end
      end

      private
      def cache_key(input)
        sort_keys = input[:sort_keys] || input[:sortby]
        drilldown_sort_keys =
          input[:drilldown_sort_keys] || input[:drilldown_sortby]
        key = "logical_select\0"
        key << "#{input[:logical_table]}\0"
        key << "#{input[:shard_key]}\0"
        key << "#{input[:min]}\0"
        key << "#{input[:min_border]}\0"
        key << "#{input[:max]}\0"
        key << "#{input[:max_border]}\0"
        key << "#{input[:filter]}\0"
        key << "#{sort_keys}\0"
        key << "#{input[:output_columns]}\0"
        key << "#{input[:offset]}\0"
        key << "#{input[:limit]}\0"
        key << "#{input[:drilldown]}\0"
        key << "#{drilldown_sort_keys}\0"
        key << "#{input[:match_columns]}\0"
        key << "#{input[:query]}\0"
        key << "#{input[:drilldown_output_columns]}\0"
        key << "#{input[:drilldown_offset]}\0"
        key << "#{input[:drilldown_limit]}\0"
        key << "#{input[:drilldown_calc_types]}\0"
        key << "#{input[:drilldown_calc_target]}\0"
        key << "#{input[:drilldown_filter]}\0"
        key << "#{input[:post_filter]}\0"
        key << "#{input[:load_table]}\0"
        key << "#{input[:load_columns]}\0"
        key << "#{input[:load_values]}\0"
        labeled_drilldowns = LabeledDrilldowns.parse(input).sort_by(&:label)
        labeled_drilldowns.each do |drilldown|
          key << "#{drilldown.label}\0"
          key << "#{drilldown.table}\0"
          key << "#{drilldown.keys.join(',')}\0"
          key << "#{drilldown.output_columns}\0"
          key << "#{drilldown.offset}\0"
          key << "#{drilldown.limit}\0"
          key << "#{drilldown.calc_types}\0"
          key << "#{drilldown.calc_target_name}\0"
          key << "#{drilldown.filter}\0"
          key << drilldown.dynamic_columns.cache_key
        end
        dynamic_columns = DynamicColumns.parse("[logical_select]", input)
        key << dynamic_columns.cache_key
        key
      end

      def resolve_target_table_name(result_set)
        target_table = result_set
        while target_table.domain_id
          domain = Context.instance[target_table.domain_id]
          break unless domain.is_a?(Table)
          target_table = domain
        end
        target_table.name
      end

      def load_records(context)
        load_table = context.load_table
        load_columns = context.load_columns
        load_values = context.load_values
        return if load_table.nil?
        return if load_columns.nil?
        return if load_values.nil?

        n_loaded_records = 0
        to = Context.instance[load_table]
        to_columns = []
        begin
          load_columns.split(/\s*,\s*/).each do |name|
            to_columns << to.find_column(name)
          end
          context.result_sets.each do |result_set|
            output_columns = result_set.parse_output_columns(load_values)
            begin
              output_columns.apply(to_columns)
            ensure
              output_columns.close
            end
            n_sub_loaded_records = result_set.size
            query_logger.log(:size,
                             ":",
                             "load(#{n_sub_loaded_records})" +
                             "[#{resolve_target_table_name(result_set)}]: " +
                             "[#{load_table}][#{to.size}]")
            n_loaded_records += n_sub_loaded_records
          end
        ensure
          to_columns.each do |column|
            column.close if column.is_a?(Accessor)
          end
        end
        query_logger.log(:size,
                         ":",
                         "load(#{n_loaded_records}): " +
                         "[#{load_table}][#{to.size}]")
      end

      def write_records(writer, context)
        result_sets = context.result_sets

        n_hits = 0
        n_elements = 2 # for N hits and columns
        result_sets.each do |result_set|
          n_hits += result_set.size
          n_elements += result_set.size
        end

        output_columns = context.output_columns

        writer.array("RESULTSET", n_elements) do
          writer.array("NHITS", 1) do
            writer.write(n_hits)
          end
          first_result_set = result_sets.first
          if first_result_set
            writer.write_table_columns(first_result_set, output_columns)
          end

          n_outputs = 0
          current_offset = context.offset
          current_offset += n_hits if current_offset < 0
          current_limit = context.limit
          current_limit += n_hits + 1 if current_limit < 0
          options = {
            :offset => current_offset,
            :limit => current_limit,
          }
          if context.sort_keys.any? {|sort_key| sort_key.start_with?("-")}
            result_sets = result_sets.reverse
          end
          result_sets.each do |result_set|
            if result_set.size > current_offset
              if context.sort_keys.empty?
                writer.write_table_records(result_set, output_columns, options)
              else
                sorted_result_set = result_set.sort(context.sort_keys, options)
                context.temporary_tables << sorted_result_set
                message = "sort(#{sorted_result_set.size}): "
                message << context.sort_keys.join(",")
                query_logger.log(:size, ":", message)
                writer.write_table_records(sorted_result_set,
                                           output_columns,
                                           offset: 0, limit: -1)
              end
              n_written = [result_set.size - current_offset, current_limit].min
              current_limit -= n_written
              n_outputs += n_written
            end
            if current_offset > 0
              current_offset = [current_offset - result_set.size, 0].max
            end
            break if current_limit <= 0
            options[:offset] = current_offset
            options[:limit] = current_limit
          end
          query_logger.log(:size, ":", "output(#{n_outputs})")
        end
      end

      def write_plain_drilldowns(writer, execute_context)
        plain_drilldown = execute_context.plain_drilldown

        drilldowns = plain_drilldown.result_sets
        sort_keys = plain_drilldown.sort_keys
        output_columns = plain_drilldown.output_columns

        drilldowns.each do |drilldown|
          options = {
            :offset => plain_drilldown.offset,
            :limit  => plain_drilldown.limit,
          }
          n_records = drilldown.size
          limit = options[:limit]
          limit += n_records + 1 if limit < 0
          offset = options[:offset]
          offset += n_records if offset < 0
          n_written = [n_records - offset, limit].min
          n_elements = 2 # for N hits and columns
          n_elements += n_written
          unless sort_keys.empty?
            drilldown = drilldown.sort(sort_keys, options)
            plain_drilldown.temporary_tables << drilldown
            message = "drilldown.sort(#{drilldown.size}): "
            message << sort_keys.join(",")
            query_logger.log(:size, ":", message)
            options = {offset: 0, limit: -1}
          end
          writer.array("RESULTSET", n_elements) do
            writer.array("NHITS", 1) do
              writer.write(n_records)
            end
            writer.write_table_columns(drilldown, output_columns)
            writer.write_table_records(drilldown, output_columns,
                                       options)
          end
          query_logger.log(:size, ":", "output.drilldown(#{n_written})")
        end
      end

      def write_labeled_drilldowns(writer, execute_context)
        labeled_drilldowns = execute_context.labeled_drilldowns
        is_command_version1 = (context.command_version == 1)

        writer.map("DRILLDOWNS", labeled_drilldowns.n_result_sets) do
          labeled_drilldowns.each do |drilldown|
            query_log_tag = "drilldowns[#{drilldown.label}]"

            writer.write(drilldown.label)

            result_set = drilldown.result_set
            n_records = result_set.size
            n_elements = 2 # for N hits and columns
            n_elements += n_records
            output_columns = drilldown.output_columns
            options = {
              :offset => drilldown.offset,
              :limit  => drilldown.limit,
            }

            limit = options[:limit]
            limit += result_set.size + 1 if limit < 0
            offset = options[:offset]
            offset += result_set.size if offset < 0
            if drilldown.sort_keys.empty?
              n_written = [n_records - offset, limit].min
            else
              result_set = result_set.sort(drilldown.sort_keys, options)
              drilldown.temporary_tables << result_set
              message = "#{query_log_tag}.sort(#{result_set.size}): "
              message << drilldown.sort_keys.join(",")
              query_logger.log(:size, ":", message)
              options = {offset: 0, limit: -1}
              n_written = result_set.size
            end
            writer.array("RESULTSET", n_elements) do
              writer.array("NHITS", 1) do
                writer.write(n_records)
              end
              writer.write_table_columns(result_set, output_columns)
              if is_command_version1 and drilldown.need_command_version2?
                context.with_command_version(2) do
                  writer.write_table_records(result_set,
                                             drilldown.output_columns_v2,
                                             options)
                end
              else
                writer.write_table_records(result_set, output_columns, options)
              end
            end
            query_logger.log(:size,
                             ":",
                             "output.#{query_log_tag}(#{n_written})")
          end
        end
      end

      module Calculatable
        def calc_target(table)
          return nil if @calc_target_name.nil?
          table.find_column(@calc_target_name)
        end

        private
        def parse_calc_types(raw_types)
          return TableGroupFlags::CALC_COUNT if raw_types.nil?

          types = 0
          raw_types.strip.split(/ *, */).each do |name|
            case name
            when "COUNT"
              types |= TableGroupFlags::CALC_COUNT
            when "MAX"
              types |= TableGroupFlags::CALC_MAX
            when "MIN"
              types |= TableGroupFlags::CALC_MIN
            when "SUM"
              types |= TableGroupFlags::CALC_SUM
            when "AVG"
              types |= TableGroupFlags::CALC_AVG
            when "NONE"
              # Do nothing
            else
              raise InvalidArgument, "invalid drilldown calc type: <#{name}>"
            end
          end
          types
        end
      end

      class ExecuteContext
        include KeysParsable

        attr_reader :enumerator
        attr_reader :match_columns
        attr_reader :query
        attr_reader :filter
        attr_reader :offset
        attr_reader :limit
        attr_reader :sort_keys
        attr_reader :output_columns
        attr_reader :post_filter
        attr_reader :load_table
        attr_reader :load_columns
        attr_reader :load_values
        attr_reader :dynamic_columns
        attr_reader :result_sets
        attr_reader :shard_targets
        attr_reader :shard_results
        attr_reader :plain_drilldown
        attr_reader :labeled_drilldowns
        attr_reader :temporary_tables
        attr_reader :expressions
        def initialize(input)
          @input = input
          @enumerator = LogicalEnumerator.new("logical_select", @input)
          @match_columns = @input[:match_columns]
          @query = @input[:query]
          @filter = @input[:filter]
          @offset = (@input[:offset] || 0).to_i
          @limit = (@input[:limit] || 10).to_i
          @sort_keys = parse_keys(@input[:sort_keys] || @input[:sortby])
          @output_columns = @input[:output_columns] || "_id, _key, *"
          @post_filter = @input[:post_filter]
          @load_table = @input[:load_table]
          @load_columns = @input[:load_columns]
          @load_values = @input[:load_values]

          @dynamic_columns = DynamicColumns.parse("[logical_select]", @input)

          @result_sets = []
          @shard_targets = []
          @shard_results = []
          @plain_drilldown = PlainDrilldownExecuteContext.new(@input)
          @labeled_drilldowns = LabeledDrilldowns.parse(@input)

          @temporary_tables = []

          @expressions = []
        end

        def close
          @plain_drilldown.close
          @labeled_drilldowns.close

          @expressions.each do |expression|
            expression.close
          end

          @temporary_tables.each do |table|
            table.close
          end
        end
      end

      class PlainDrilldownExecuteContext
        include KeysParsable
        include Calculatable

        attr_reader :keys
        attr_reader :offset
        attr_reader :limit
        attr_reader :sort_keys
        attr_reader :output_columns
        attr_reader :calc_target_name
        attr_reader :calc_types
        attr_reader :filter
        attr_reader :result_sets
        attr_reader :temporary_tables
        attr_reader :expressions
        def initialize(input)
          @input = input
          @keys = parse_keys(@input[:drilldown])
          @offset = (@input[:drilldown_offset] || 0).to_i
          @limit = (@input[:drilldown_limit] || 10).to_i
          @sort_keys = parse_keys(@input[:drilldown_sort_keys] ||
                                  @input[:drilldown_sortby])
          @output_columns = @input[:drilldown_output_columns]
          @output_columns ||= "_key, _nsubrecs"
          @calc_target_name = @input[:drilldown_calc_target]
          @calc_types = parse_calc_types(@input[:drilldown_calc_types])
          @filter = @input[:drilldown_filter]

          @result_sets = []

          @temporary_tables = []

          @expressions = []
        end

        def close
          @temporary_tables.each do |table|
            table.close
          end

          @expressions.each do |expression|
            expression.close
          end
        end

        def have_keys?
          @keys.size > 0
        end

        def n_result_sets
          @result_sets.size
        end
      end

      class LabeledDrilldowns
        include Enumerable
        include TSort

        class << self
          def parse(input)
            contexts = {}
            labeled_arguments = LabeledArguments.new(input, /drilldowns?/)
            labeled_arguments.each do |label, arguments|
              next if arguments["keys"].nil?
              context = LabeledDrilldownExecuteContext.new(label, arguments)
              contexts[label] = context
            end
            new(contexts)
          end
        end

        def initialize(contexts)
          @contexts = contexts
          @dependencies = {}
          @contexts.each do |label, context|
            if context.table
              depended_context = @contexts[context.table]
              if depended_context.nil?
                raise "Unknown drilldown: <#{context.table}>"
              end
              @dependencies[label] = [depended_context]
            else
              @dependencies[label] = []
            end
          end
        end

        def close
          @contexts.each_value do |context|
            context.close
          end
        end

        def [](label)
          @contexts[label]
        end

        def have_keys?
          not @contexts.empty?
        end

        def n_result_sets
          @contexts.size
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

      class LabeledDrilldownExecuteContext
        include KeysParsable
        include Calculatable

        attr_reader :label
        attr_reader :keys
        attr_reader :offset
        attr_reader :limit
        attr_reader :sort_keys
        attr_reader :output_columns
        attr_reader :calc_target_name
        attr_reader :calc_types
        attr_reader :filter
        attr_reader :table
        attr_reader :dynamic_columns
        attr_accessor :result_set
        attr_reader :temporary_tables
        attr_reader :expressions
        def initialize(label, parameters)
          @label = label
          @keys = parse_keys(parameters["keys"])
          @offset = (parameters["offset"] || 0).to_i
          @limit = (parameters["limit"] || 10).to_i
          @sort_keys = parse_keys(parameters["sort_keys"] ||
                                  parameters["sortby"])
          @output_columns = parameters["output_columns"]
          @output_columns ||= "_key, _nsubrecs"
          @calc_target_name = parameters["calc_target"]
          @calc_types = parse_calc_types(parameters["calc_types"])
          @filter = parameters["filter"]
          @table = parameters["table"]

          tag = "[logical_select][drilldowns][#{@label}]"
          @dynamic_columns = DynamicColumns.parse(tag, parameters)

          @result_set = nil

          @temporary_tables = []

          @expressions = []
        end

        def close
          @expressions.each do |expression|
            expression.close
          end

          @temporary_tables.each do |table|
            table.close
          end
        end

        def need_command_version2?
          /[.\[]/ === @output_columns
        end

        def output_columns_v2
          columns = @output_columns.strip.split(/ *, */)
          converted_columns = columns.collect do |column|
            match_data = /\A_value\.(.+)\z/.match(column)
            if match_data.nil?
              column
            else
              nth_key = keys.index(match_data[1])
              if nth_key
                "_key[#{nth_key}]"
              else
                column
              end
            end
          end
          converted_columns.join(",")
        end
      end

      class Executor
        include QueryLoggable

        def initialize(context)
          @context = context
        end

        def execute
          execute_search
          if @context.plain_drilldown.have_keys?
            execute_plain_drilldown
          elsif @context.labeled_drilldowns.have_keys?
            execute_labeled_drilldowns
          end
        end

        private
        def execute_search
          first_shard = nil
          enumerator = @context.enumerator
          enumerator.each do |shard, shard_range|
            first_shard ||= shard
            shard_executor = ShardExecutor.new(@context, shard, shard_range)
            shard_executor.execute_pre
          end
          if first_shard.nil?
            message =
              "[logical_select] no shard exists: " +
              "logical_table: <#{enumerator.logical_table}>: " +
              "shard_key: <#{enumerator.shard_key_name}>"
            raise InvalidArgument, message
          end

          if @context.dynamic_columns.have_initial?
            targets = []
            @context.shard_targets.each do |_, target_table|
              targets << [target_table]
            end
            @context.dynamic_columns.apply_initial(targets)
          end
          @context.shard_targets.each do |shard_executor, target_table|
            shard_executor.execute
          end

          if @context.shard_results.empty?
            result_set = HashTable.create(:flags => ObjectFlags::WITH_SUBREC,
                                          :key_type => first_shard.table)
            @context.temporary_tables << result_set
            targets = [[result_set]]
            @context.dynamic_columns.apply_initial(targets)
            @context.dynamic_columns.apply_filtered(targets)
            @context.result_sets << result_set
          else
            targets = []
            @context.shard_results.each do |_, result_set, condition|
              targets << [result_set, {condition: condition}]
            end
            @context.dynamic_columns.apply_filtered(targets)
            @context.shard_results.each do |shard_executor, result_set, _|
              shard_executor.execute_post(result_set)
            end
          end
        end

        def execute_plain_drilldown
          drilldown = @context.plain_drilldown
          query_log_prefix = "drilldown"
          group_result = TableGroupResult.new
          begin
            group_result.key_begin = 0
            group_result.key_end = 0
            group_result.limit = 1
            group_result.flags = drilldown.calc_types
            drilldown.keys.each do |key|
              @context.result_sets.each do |result_set|
                with_calc_target(group_result,
                                 drilldown.calc_target(result_set)) do
                  result_set.group([key], group_result)
                end
              end
              result_set = group_result.table
              query_logger.log(:size,
                               ":",
                               "#{query_log_prefix}(#{result_set.size})")
              result_set = apply_drilldown_filter(query_log_prefix,
                                                  drilldown,
                                                  result_set)
              drilldown.temporary_tables << result_set
              group_result.table = nil
              drilldown.result_sets << result_set
            end
          ensure
            group_result.close
          end
        end

        def execute_labeled_drilldowns
          drilldowns = @context.labeled_drilldowns

          drilldowns.tsort_each do |drilldown|
            query_log_prefix = "drilldowns[#{drilldown.label}]"
            group_result = TableGroupResult.new
            keys = drilldown.keys
            begin
              group_result.key_begin = 0
              group_result.key_end = keys.size - 1
              if keys.size > 1
                group_result.max_n_sub_records = 1
              end
              group_result.limit = 1
              group_result.flags = drilldown.calc_types
              if drilldown.table
                target_table = drilldowns[drilldown.table].result_set
                with_calc_target(group_result,
                                 drilldown.calc_target(target_table)) do
                  target_table.group(keys, group_result)
                end
              else
                @context.result_sets.each do |result_set|
                  with_calc_target(group_result,
                                   drilldown.calc_target(result_set)) do
                    result_set.group(keys, group_result)
                  end
                end
              end
              result_set = group_result.table
              query_logger.log(:size,
                               ":",
                               "#{query_log_prefix}(#{result_set.size})")
              options = {query_log_prefix: "#{query_log_prefix}."}
              drilldown.dynamic_columns.apply_initial([[result_set]],
                                                      options)
              result_set = apply_drilldown_filter(query_log_prefix,
                                                  drilldown,
                                                  result_set)
              drilldown.temporary_tables << result_set
              group_result.table = nil
              drilldown.result_set = result_set
            ensure
              group_result.close
            end
          end
        end

        def with_calc_target(group_result, calc_target)
          group_result.calc_target = calc_target
          begin
            yield
          ensure
            calc_target.close if calc_target
            group_result.calc_target = nil
          end
        end

        def apply_drilldown_filter(query_log_prefix, drilldown, result_set)
          filter = drilldown.filter
          return result_set if filter.nil?

          expression = Expression.create(result_set)
          drilldown.expressions << expression
          expression.parse(filter)
          filtered_result_set = result_set.select(expression)
          drilldown.temporary_tables << result_set
          n_records = filtered_result_set.size
          query_logger.log(:size,
                           ":",
                           "#{query_log_prefix}.filter(#{n_records})")
          filtered_result_set
        end
      end

      class ShardExecutor
        include QueryLoggable

        def initialize(context, shard, shard_range)
          @context = context
          @shard = shard
          @shard_range = shard_range

          @target_table = @shard.table

          @match_columns = @context.match_columns
          @query = @context.query
          @filter = @context.filter
          @post_filter = @context.post_filter
          @sort_keys = @context.sort_keys
          @result_sets = @context.result_sets
          @shard_targets = @context.shard_targets
          @shard_results = @context.shard_results
          @temporary_tables = @context.temporary_tables

          @target_range = @context.enumerator.target_range

          @cover_type = @target_range.cover_type(@shard_range)
        end

        def execute_pre
          return if @cover_type == :none
          return if @target_table.empty?

          shard_key = @shard.key
          if shard_key.nil?
            message = "[logical_select] shard_key doesn't exist: " +
                      "<#{@shard.key_name}>"
            raise InvalidArgument, message
          end

          if @context.dynamic_columns.have_initial?
            if @target_table == @shard.table
              if @cover_type == :all
                @target_table = @target_table.select_all
              else
                expression_builder = RangeExpressionBuilder.new(shard_key,
                                                                @target_range)
                expression = create_expression(@target_table)
                expression_builder.build(expression, @shard_range)
                @target_table = @target_table.select(expression)
                @cover_type = :all
              end
              @temporary_tables << @target_table
            end
          end
          @shard_targets << [self, @target_table]
        end

        def execute
          create_expression_builder(@shard.key) do |expression_builder|
            case @cover_type
            when :all
              filter_shard_all(expression_builder)
            when :partial_min
              filter_table do |expression|
                expression_builder.build_partial_min(expression)
              end
            when :partial_max
              filter_table do |expression|
                expression_builder.build_partial_max(expression)
              end
            when :partial_min_and_max
              filter_table do |expression|
                expression_builder.build_partial_min_and_max(expression)
              end
            end
          end
        end

        def execute_post(result_set)
          if @post_filter
            result_set = apply_post_filter(result_set)
            @temporary_tables << result_set
          end
          @result_sets << result_set
        end

        private
        def filter_shard_all(expression_builder)
          if @query.nil? and @filter.nil?
            @temporary_tables.delete(@target_table)
            add_result(@target_table, nil)
          else
            filter_table do |expression|
              expression_builder.build_all(expression)
            end
          end
        end

        def create_expression(table)
          expression = Expression.create(table)
          @context.expressions << expression
          expression
        end

        def create_expression_builder(shard_key)
          expression_builder = RangeExpressionBuilder.new(shard_key,
                                                          @target_range)
          expression_builder.match_columns = @match_columns
          expression_builder.query = @query
          expression_builder.filter = @filter
          begin
            yield(expression_builder)
          ensure
            expression = expression_builder.match_columns_expression
            @context.expressions << expression if expression
          end
        end

        def filter_table
          table = @target_table
          expression = create_expression(table)
          yield(expression)
          add_result(table.select(expression), expression)
        end

        def apply_post_filter(table)
          expression = create_expression(table)
          expression.parse(@post_filter)
          table.select(expression)
        end

        def add_result(result_set, condition)
          query_logger.log(:size, ":",
                           "select(#{result_set.size})[#{@shard.table_name}]")

          if result_set.empty?
            result_set.close
            return
          end

          if result_set == @shard.table
            if @context.dynamic_columns.have_filtered?
              result_set = result_set.select_all
              @temporary_tables << result_set
            end
          else
            @temporary_tables << result_set
          end

          @shard_results << [self, result_set, condition]
        end
      end
    end
  end
end
