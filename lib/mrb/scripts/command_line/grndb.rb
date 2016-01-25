module Groonga
  module CommandLine
    class Grndb
      def initialize(argv)
        @program_path, *@arguments = argv
        @succeeded = true
        @database_path = nil
      end

      def run
        command_line_parser = create_command_line_parser
        options = nil
        begin
          options = command_line_parser.parse(@arguments)
        rescue Slop::Error => error
          $stderr.puts(error.message)
          $stderr.puts
          $stderr.puts(command_line_parser.help_message)
          return false
        end
        @succeeded
      end

      private
      def create_command_line_parser
        program_name = File.basename(@program_path)
        parser = CommandLineParser.new(program_name)

        parser.add_command("check") do |command|
          command.description = "Check database"

          options = command.options
          options.banner += " DB_PATH"
          options.string("--target", "Check only the target object.")

          command.add_action do |options|
            open_database(command, options) do |database, rest_arguments|
              check(database, options, rest_arguments)
            end
          end
        end

        parser.add_command("recover") do |command|
          command.description = "Recover database"

          options = command.options
          options.banner += " DB_PATH"

          command.add_action do |options|
            open_database(command, options) do |database, rest_arguments|
              recover(database, options, rest_arguments)
            end
          end
        end

        parser
      end

      def open_database(command, options)
        arguments = options.arguments
        if arguments.empty?
          $stderr.puts("Database path is missing")
          $stderr.puts
          $stderr.puts(command.help_message)
          @succeesed = false
          return
        end

        database = nil
        @database_path, *rest_arguments = arguments
        begin
          database = Database.open(@database_path)
        rescue Error => error
          $stderr.puts("Failed to open database: <#{@database_path}>")
          $stderr.puts(error.message)
          @succeeded = false
          return
        end

        begin
          yield(database, rest_arguments)
        ensure
          database.close
        end
      end

      def failed(*messages)
        messages.each do |message|
          $stderr.puts(message)
        end
        @succeeded = false
      end

      def recover(database, options, arguments)
        begin
          database.recover
        rescue Error => error
          failed("Failed to recover database: <#{@database_path}>",
                 error.message)
        end
      end

      def check(database, options, arguments)
        if database.locked?
          message =
            "Database is locked. " +
            "It may be broken. " +
            "Re-create the database."
          failed(message)
        end

        target_name = options[:target]
        if target_name
          check_one(database, target_name, options, arguments)
        else
          check_all(database, options, arguments)
        end
      end

      def check_object(object, options, arguments)
        case object
        when IndexColumn
          return unless object.locked?
          message =
            "[#{object.name}] Index column is locked. " +
            "It may be broken. " +
            "Re-create index by '#{@program_path} recover #{@database_path}'."
          failed(message)
        when Column
          return unless object.locked?
          name = object.name
          message =
            "[#{name}] Data column is locked. " +
            "It may be broken. " +
            "(1) Truncate the column (truncate #{name}) or " +
            "clear lock of the column (lock_clear #{name}) " +
            "and (2) load data again."
          failed(message)
        when Table
          return unless object.locked?
          name = object.name
          message =
            "[#{name}] Table is locked. " +
            "It may be broken. " +
            "(1) Truncate the table (truncate #{name}) or " +
            "clear lock of the table (lock_clear #{name}) " +
            "and (2) load data again."
          failed(message)
        end
      end

      def failed_to_open(name)
        message =
          "[#{name}] Can't open object. " +
          "It's broken. " +
          "Re-create the object or the database."
        failed(message)
      end

      def check_one(database, target_name, options, arguments)
        context = Context.instance

        target = context[target_name]
        if target.nil?
          exist_p = open_cursor(database) do |cursor|
            cursor.any? do
              cursor.key == target_name
            end
          end
          if exist_p
            failed_to_open(target_name)
          else
            message = "[#{target_name}] Not exist."
            failed(message)
          end
          return
        end

        check_object_recursive(database, target, options, arguments)
      end

      def check_object_recursive(database, target, options, arguments)
        context = Context.instance

        check_object(target, options, arguments)
        case target
        when Table
          target.column_ids.each do |column_id|
            column = context[column_id]
            if column.nil?
              record = Record.new(database, column_id)
              failed_to_open(record.key)
            else
              check_object(column, options, arguments)
            end
          end
        when FixedSizeColumn, VariableSizeColumn
          range_id = target.range_id
          range = context[range_id]
          if range.nil?
            record = Record.new(database, range_id)
            failed_to_open(record.key)
          elsif range.is_a?(Table)
            check_object_recursive(database, range, options, arguments)
          end
        end
      end

      def check_all(database, options, arguments)
        open_cursor(database) do |cursor|
          context = Context.instance
          cursor.each do |id|
            next if ID.builtin?(id)
            next if context[id]
            failed_to_open(cursor.key)
          end
        end

        database.each do |object|
          check_object(object, options, arguments)
        end
      end

      def open_cursor(database, &block)
        flags =
          TableCursorFlags::ASCENDING |
          TableCursorFlags::BY_ID
        TableCursor.open(database, :flags => flags, &block)
      end
    end
  end
end
