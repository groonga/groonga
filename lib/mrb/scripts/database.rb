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

    def each_table(options={})
      context = Context.instance
      min = options[:prefix]
      flags =
        TableCursorFlags::ASCENDING |
        TableCursorFlags::BY_KEY
      flags |= TableCursorFlags::PREFIX if min
      cursor = TableCursor.open(self, :min => min, :flags => flags)
      begin
        cursor.each do |id|
          object = context[id]
          yield(object) if object.is_a?(Table)
        end
      ensure
        cursor.close
      end
    end
  end
end
