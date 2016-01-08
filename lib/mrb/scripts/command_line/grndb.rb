module Groonga
  module CommandLine
    class Grndb
      def initialize(argv)
        @command, *@arguments = argv
        @succeeded = true
        @executed = false
        @database_path = nil
      end

      def run
        slop = create_slop
        rest = nil
        begin
          rest = slop.parse(@arguments)
        rescue Slop::Error
          $stderr.puts($!.message)
          return false
        end

        if slop.help?
          $stdout.puts(slop.help)
          return true
        end

        unless @executed
          if rest.empty?
            $stderr.puts("No command is specified.")
          else
            $stderr.puts("Unknown command: <#{rest.first}>")
          end
          return false
        end

        @succeeded
      end

      private
      def create_slop
        slop = Slop.new
        command_name = File.basename(@command)
        slop.banner = "Usage: #{command_name} COMMAND [OPTIONS] DB_PATH"
        slop_enable_help(slop)

        slop.command "check" do |command|
          command.description "Check database"
          slop_enable_help(command)

          command.on("--target=", "Check only the target object.")

          command.run do |options, arguments|
            run_command(options, arguments) do |database, new_arguments|
              check(database, options, new_arguments)
            end
          end
        end

        slop.command "recover" do |command|
          command.description "Recover database"
          slop_enable_help(command)

          command.run do |options, arguments|
            run_command(options, arguments) do |database, new_arguments|
              recover(database, options, new_arguments)
            end
          end
        end

        slop
      end

      def slop_enable_help(slop)
        slop.on("-h", "--help", "Display this help message.", :tail => true)
      end

      def open_database(arguments)
        if arguments.empty?
          $stderr.puts("Database path is missing")
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

      def run_command(options, arguments)
        @executed = true

        if options.help?
          $stdout.puts(options.help)
          return
        end

        open_database(arguments) do |database|
          yield(database)
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
            "Re-create index by '#{@command} recover #{@database_path}'."
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
