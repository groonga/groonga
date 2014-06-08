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

    def match_resolve_index_db_obj(db_obj)
      index_info = db_obj.find_index(op)
      return if index_info.nil?
      put_index(index_info.index, index_info.section_id, 1)
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
