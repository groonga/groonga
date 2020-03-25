module Groonga
  class ScanInfoSearchIndex < Struct.new(:index_column,
                                         :section_id,
                                         :weight,
                                         :scorer,
                                         :scorer_args_expr,
                                         :scorer_args_expr_offset)
    def unlink
      index_column.unlink if index_column
    end
  end
end
