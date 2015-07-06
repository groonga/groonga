module Groonga
  class Record
    attr_reader :table
    attr_reader :id

    def inspect
      super.gsub(/>\z/) do
        "@id=#{@id.inspect}, @table=#{@table.inspect}>"
      end
    end
  end
end
