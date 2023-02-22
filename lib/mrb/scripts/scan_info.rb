module Groonga
  class ScanInfo
    module Flags
      ACCESSOR  = 0x01
      PUSH      = 0x02
      POP       = 0x04
      PRE_CONST = 0x08
    end

    def apply(data)
      self.op = data.op
      self.logical_op = data.logical_op
      self.weight_factor = data.weight_factor
      self.end = data.end
      self.query = data.query
      self.flags = data.flags
      if data.max_interval
        self.max_interval = data.max_interval
      end
      if data.additional_last_interval
        self.additional_last_interval = data.additional_last_interval
      end
      if data.max_element_intervals
        self.max_element_intervals = data.max_element_intervals
      end
      if data.min_interval
        self.min_interval = data.min_interval
      end
      if data.similarity_threshold
        self.similarity_threshold = data.similarity_threshold
      end
      if data.quorum_threshold
        self.quorum_threshold = data.quorum_threshold
      end
      data.args.each do |arg|
        push_arg(arg)
      end
      data.search_indexes.each do |search_index|
        put_index(search_index.index_column,
                  search_index.section_id,
                  search_index.weight,
                  search_index.scorer,
                  search_index.scorer_args_expr,
                  search_index.scorer_args_expr_offset || 0)
      end
      if data.start_position
        self.start_position = data.start_position
      end
    end
  end
end
