module Groonga
  class Database
    def each
      context = Context.instance
      flags =
        TableCursorFlags::ASCENDING |
        TableCursorFlags::BY_ID
      cursor = TableCursor.open(self, :flags => flags)
      begin
        cursor.each do |id|
          object = context[id]
          yield(object) if object
        end
      ensure
        cursor.close
      end
    end
  end
end
