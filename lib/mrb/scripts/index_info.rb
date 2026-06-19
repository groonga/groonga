module Groonga
  class IndexInfo
    attr_reader :index
    attr_reader :section_id
    attr_reader :start_position
    def initialize(index, section_id, start_position)
      @index = index
      @section_id = section_id
      @start_position = start_position
    end
  end
end
