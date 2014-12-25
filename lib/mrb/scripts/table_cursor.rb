module Groonga
  class TableCursor
    def each
      loop do
        id = self.next
        p [id]
        p Groonga::ID::NIL
        return if id == Groonga::ID::NIL
        yield(id)
      end
    end
  end
end
