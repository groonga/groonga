module Groonga
  class TableCursor
    def each
      loop do
        id = self.next
        return if id == Groonga::ID::NIL
        yield(id)
      end
    end
  end
end
