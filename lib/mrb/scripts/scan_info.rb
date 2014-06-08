module Groonga
  class ScanInfo
    def resolve_index_db_obj(db_obj)
      index_info = db_obj.find_index(op)
      return if index_info.nil?
      put_index(index_info.index, index_info.section_id, 1)
    end
  end
end
