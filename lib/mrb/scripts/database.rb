module Groonga
  class Database
    def each
      context = Context.instance
      cursor = TableCursor.open(self)
      begin
        cursor.each do |id|
          object = context[id];
          yield(object) if object
        end
      ensure
        cursor.close
      end
    end
  end
end
