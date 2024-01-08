module Groonga
  class ScanInfoSearchIndex < Struct.new(:index_column,
                                         :section_id,
                                         :start_position,
                                         :weight,
                                         :scorer,
                                         :scorer_args_expr,
                                         :scorer_args_expr_offset)
    def unref
      index_column.unref if index_column
    end
  end
end
