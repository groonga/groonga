module Groonga
  class ScanInfo
    module Flags
      ACCESSOR  = 0x01
      PUSH      = 0x02
      POP       = 0x04
      PRE_CONST = 0x08
    end

    def each_arg
      i = 0
      loop do
        arg = get_arg(i)
        break if arg.nil?
        yield(arg)
        i += 1
      end
      nil
    end

    def match_resolve_index
      each_arg do |arg|
        case arg
        when Expression
          match_resolve_index_expression(arg)
        when Accessor
          match_resolve_index_accessor(arg)
        when Object
          match_resolve_index_db_obj(arg)
        else
          self.query = arg
        end
      end
    end

    def match_resolve_index_expression(expression)
      codes = expression.codes
      n_codes = codes.size
      i = 0
      while i < n_codes
        code = codes[i]
        value = code.value
        case value
        when Groonga::Accessor
          match_resolve_index_expression_accessor(code)
        when Groonga::FixedSizeColumn, Groonga::VariableSizeColumn
          match_resolve_index_expression_data_column(code)
        when Groonga::IndexColumn
          section_id = 0
          rest_n_codes = n_codes - i
          if rest_n_codes >= 2 and
              codes[i + 1].value.is_a?(Groonga::Bulk) and
              codes[i + 1].value.domain == Groonga::ID::UINT32 and
              codes[i + 2].op == Groonga::Operator::GET_MEMBER
            section_id = codes[i + 1].value.value + 1
            code = codes[i + 2]
            i += 2
          end
          put_index(value, section_id, code.weight)
        end
        i += 1
      end
    rescue => exception
      p exception
      p exception.class
      puts exception.backtrace
    end

    def match_resolve_index_expression_accessor(expr_code)
      accessor = expr_code.value
      self.flags |= Flags::ACCESSOR
      index_info = accessor.find_index(op)
      return if index_info.nil?
      if accessor.next
        put_index(accessor, index_info.section_id, expr_code.weight)
      else
        put_index(index_info.index, index_info.section_id, expr_code.weight)
      end
    end

    def match_resolve_index_expression_data_column(expr_code)
      column = expr_code.value
      index_info = column.find_index(op)
      return if index_info.nil?
      put_index(index_info.index, index_info.section_id, expr_code.weight)
    end

    def match_resolve_index_db_obj(db_obj)
      index_info = db_obj.find_index(op)
      return if index_info.nil?
      put_index(index_info.index, index_info.section_id, 1)
    end

    def match_resolve_index_accessor(accessor)
      self.flags |= Flags::ACCESSOR
      index_info = accessor.find_index(op)
      return if index_info.nil?
      if accessor.next
        put_index(accessor, index_info.section_id, 1)
      else
        put_index(index_info.index, index_info.section_id, 1)
      end
    end

    def call_relational_resolve_indexes
      # better index resolving framework for functions should be implemented
      each_arg do |arg|
        call_relational_resolve_index(arg)
      end
    end

    private
    def call_relational_resolve_index(object)
      case object
      when Accessor
        call_relational_resolve_index_accessor(object)
      when Bulk
        self.query = object
      else
        call_relational_resolve_index_db_obj(object)
      end
    end

    def call_relational_resolve_index_db_obj(db_obj)
      index_info = db_obj.find_index(op)
      return if index_info.nil?
      put_index(index_info.index, index_info.section_id, 1)
    end

    def call_relational_resolve_index_accessor(accessor)
      self.flags |= Flags::ACCESSOR
      index_info = accessor.find_index(op)
      return if index_info.nil?
      put_index(index_info.index, index_info.section_id, 1)
    end
  end
end
