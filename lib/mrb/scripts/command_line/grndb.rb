require "groonga-log"

module Groonga
  module CommandLine
    class Grndb
      include Loggable

      def initialize(argv)
        @program_path, *@arguments = argv
        @output = LocaleOutput.new($stderr)
        @succeeded = true
        @database_path = nil
      end

      def run
        command_line_parser = create_command_line_parser
        options = nil
        begin
          options = command_line_parser.parse(@arguments)
        rescue Slop::Error => error
          @output.puts(error.message)
          @output.puts
          @output.puts(command_line_parser.help_message)
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
          options.array("--groonga-log-path",
                        "Path to Groonga log file to be checked.",
                        "You can specify multiple times to specify multiple log files.")

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
          options.boolean("--force-truncate",
                          "Force to truncate corrupted objects.")
          options.boolean("--force-lock-clear",
                          "Force to clear lock of locked objects.")

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
          @output.puts("Database path is missing")
          @output.puts
          @output.puts(command.help_message)
          @succeesed = false
          return
        end

        database = nil
        @database_path, *rest_arguments = arguments
        begin
          database = Database.open(@database_path)
        rescue Error => error
          @output.puts("Failed to open database: <#{@database_path}>")
          @output.puts(error.message)
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
          @output.puts(message)
          logger.log(:error, message)
        end
        @succeeded = false
      end

      def recover(database, options, arguments)
        recoverer = Recoverer.new(@output)
        recoverer.database = database
        recoverer.force_truncate = options[:force_truncate]
        recoverer.force_lock_clear = options[:force_lock_clear]
        begin
          recoverer.recover
        rescue Error => error
          failed("Failed to recover database: <#{@database_path}>",
                 error.message)
        end
      end

      def check(database, options, arguments)
        logger.log(:info, "Checking database: <#{@database_path}>")
        checker = Checker.new(@output)
        checker.program_path = @program_path
        checker.database_path = @database_path
        checker.database = database
        checker.log_paths = options[:groonga_log_path]
        checker.on_failure = lambda do |message|
          failed(message)
        end

        checker.check_log_paths

        checker.check_database

        target_name = options[:target]
        if target_name
          checker.check_one(target_name)
        else
          checker.check_all
        end
        logger.log(:info, "Checked database: <#{@database_path}>")
      end

      class Checker
        include Loggable

        attr_writer :program_path
        attr_writer :database_path
        attr_writer :database
        attr_writer :log_paths
        attr_writer :on_failure

        def initialize(output)
          @output = output
          @context = Context.instance
          @checked = {}
        end

        def check_log_paths
          @log_paths.each do |log_path|
            begin
              log_file = File.new(log_path)
            rescue => error
              message = "[#{log_path}] Can't open Groonga log path: "
              message << "#{error.class}: #{error.message}"
              failed(message)
              next
            end

            begin
              check_log_file(log_file)
            ensure
              log_file.close
            end
          end
        end

        def check_database
          check_database_orphan_inspect
          check_database_locked
          check_database_corrupt
          check_database_dirty
          check_database_empty_files
        end

        def check_one(target_name)
          logger.log(:info, "Checking object: <#{target_name}>")
          target = @context[target_name]
          if target.nil?
            exist_p = open_database_cursor do |cursor|
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

          check_object_recursive(target)
          logger.log(:info, "Checked object: <#{target_name}>")
        end

        def check_all
          open_database_cursor do |cursor|
            cursor.each do |id|
              next if ID.builtin?(id)
              next if builtin_object_name?(cursor.key)
              next if @context[id]
              failed_to_open(cursor.key)
            end
          end

          @database.each do |object|
            check_object(object)
          end
        end

        private
        def check_log_file(log_file)
          return # Disable for now

          parser = GroongaLog::Parser.new
          parser.parse(log_file) do |statistic|
            p statistic.to_h
          end
        end

        def check_database_orphan_inspect
          found = false
          open_database_cursor do |cursor|
            cursor.each do |id|
              if cursor.key == "inspect" and @context[id].nil?
                message =
                  "Database has orphan 'inspect' object. " +
                  "Remove it by '#{@program_path} recover #{@database_path}'."
                found = true
                failed(message)
                break
              end
            end
          end
          unless found
            logger.log(:info,
                       "Database doesn't have orphan 'inspect' object: " +
                       "<#{@database_path}>")
          end
        end

        def check_database_locked
          unless @database.locked?
            logger.log(:info, "Database is not locked: <#{@database_path}>")
            return
          end

          message =
            "Database is locked. " +
            "It may be broken. " +
            "Re-create the database."
          failed(message)
        end

        def check_database_corrupt
          unless @database.corrupt?
            logger.log(:info, "Database is not corrupted: <#{@database_path}>")
            return
          end

          message =
            "Database is corrupt. " +
            "Re-create the database."
          failed(message)
        end

        def check_database_dirty
          unless @database.dirty?
            logger.log(:info, "Database is not dirty: <#{@database_path}>")
            return
          end

          last_modified = @database.last_modified
          if File.stat(@database.path).mtime > last_modified
            return
          end

          open_database_cursor do |cursor|
            cursor.each do |id|
              next if ID.builtin?(id)
              path = "%s.%07x" % [@database.path, id]
              next unless File.exist?(path)
              return if File.stat(path).mtime > last_modified
            end
          end

          message =
            "Database wasn't closed successfully. " +
            "It may be broken. " +
            "Re-create the database."
          failed(message)
        end

        def check_database_empty_files
          dirname = File.dirname(@database.path)
          basename = File.basename(@database.path)

          Dir.entries(dirname).sort.each do |path|
            next unless path.start_with?(basename)

            full_path = "#{dirname}/#{path}"
            if File.stat(full_path).size.zero?
              failed("Empty file exists: <#{full_path}>")
            end
          end
        end

        def check_object(object)
          return if @checked.key?(object.id)
          @checked[object.id] = true

          check_object_locked(object)
          check_object_corrupt(object)
        end

        def check_object_locked(object)
          case object
          when IndexColumn
            unless object.locked?
              logger.log(:info, "[#{object.name}] Index column is not locked")
              return
            end
            message =
              "[#{object.name}] Index column is locked. " +
              "It may be broken. " +
              "Re-create index by '#{@program_path} recover #{@database_path}'."
            failed(message)
          when Column
            unless object.locked?
              logger.log(:info, "[#{object.name}] Column is not locked")
              return
            end
            name = object.name
            message =
              "[#{name}] Data column is locked. " +
              "It may be broken. " +
              "(1) Truncate the column (truncate #{name}) or " +
              "clear lock of the column (lock_clear #{name}) " +
              "and (2) load data again."
            failed(message)
          when Table
            unless object.locked?
              logger.log(:info, "[#{object.name}] Table is not locked")
              return
            end
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

        def check_object_corrupt(object)
          case object
          when IndexColumn
            unless object.corrupt?
              logger.log(:info, "[#{object.name}] Index column is not corrupted")
              return
            end
            message =
              "[#{object.name}] Index column is corrupt. " +
              "Re-create index by '#{@program_path} recover #{@database_path}'."
            failed(message)
          when Column
            unless object.corrupt?
              logger.log(:info, "[#{object.name}] Column is not corrupted")
              return
            end
            name = object.name
            message =
              "[#{name}] Data column is corrupt. " +
              "(1) Truncate the column (truncate #{name} or " +
              "'#{@program_path} recover --force-truncate #{@database_path}') " +
              "and (2) load data again."
            failed(message)
          when Table
            unless object.corrupt?
              logger.log(:info, "[#{object.name}] Table is not corrupted")
              return
            end
            name = object.name
            message =
              "[#{name}] Table is corrupt. " +
              "(1) Truncate the table (truncate #{name} or " +
              "'#{@program_path} recover --force-truncate #{@database_path}') " +
              "and (2) load data again."
            failed(message)
          end
        end

        def check_object_recursive(target)
          return if @checked.key?(target.id)

          check_object(target)
          case target
          when Table
            unless target.is_a?(Groonga::Array)
              domain_id = target.domain_id
              domain = @context[domain_id]
              if domain.nil?
                record = Record.new(@database, domain_id)
                failed_to_open(record.key)
              elsif domain.is_a?(Table)
                check_object_recursive(domain)
              end
            end

            target.column_ids.each do |column_id|
              column = @context[column_id]
              if column.nil?
                record = Record.new(@database, column_id)
                failed_to_open(record.key)
              else
                check_object_recursive(column)
              end
            end
          when FixedSizeColumn, VariableSizeColumn
            range_id = target.range_id
            range = @context[range_id]
            if range.nil?
              record = Record.new(@database, range_id)
              failed_to_open(record.key)
            elsif range.is_a?(Table)
              check_object_recursive(range)
            end

            lexicon_ids = []
            target.indexes.each do |index_info|
              index = index_info.index
              lexicon_ids << index.domain_id
              check_object(index)
            end
            lexicon_ids.uniq.each do |lexicon_id|
              lexicon = @context[lexicon_id]
              if lexicon.nil?
                record = Record.new(@database, lexicon_id)
                failed_to_open(record.key)
              else
                check_object(lexicon)
              end
            end
          when IndexColumn
            range_id = target.range_id
            range = @context[range_id]
            if range.nil?
              record = Record.new(@database, range_id)
              failed_to_open(record.key)
              return
            end
            check_object(range)

            target.source_ids.each do |source_id|
              source = @context[source_id]
              if source.nil?
                record = Record.new(database, source_id)
                failed_to_open(record.key)
              elsif source.is_a?(Column)
                check_object_recursive(source)
              end
            end
          end
        end

        def open_database_cursor(&block)
          flags =
            TableCursorFlags::ASCENDING |
            TableCursorFlags::BY_ID
          TableCursor.open(@database, :flags => flags, &block)
        end

        def builtin_object_name?(name)
          case name
          when "inspect"
            # Just for compatibility. It's needed for users who used
            # Groonga master at between 2016-02-03 and 2016-02-26.
            true
          else
            false
          end
        end

        def failed(message)
          @on_failure.call(message)
        end

        def failed_to_open(name)
          message =
            "[#{name}] Can't open object. " +
            "It's broken. " +
            "Re-create the object or the database."
          failed(message)
        end
      end

      class Recoverer
        include Loggable

        attr_writer :database
        attr_writer :force_truncate
        attr_writer :force_lock_clear

        def initialize(output)
          @output = output
          @context = Context.instance
        end

        def recover
          logger.log(:info, "Recovering database: <#{@database.path}>")
          if @force_truncate
            @database.each do |object|
              next unless truncate_target?(object)
              truncate_broken_object(object)
            end
          end
          if @force_lock_clear
            clear_locks
          end
          remove_empty_files
          @database.recover
          logger.log(:info, "Recovered database: <#{@database.path}>")
        end

        private
        def truncate_target?(object)
          return true if object.corrupt?

          case object
          when IndexColumn
            # It'll be recovered later.
            false
          when Column, Table
            object.locked?
          else
            false
          end
        end

        def truncate_broken_object(object)
          name = object.name
          object_path = object.path
          object_dirname = File.dirname(object_path)
          object_basename = File.basename(object_path)
          object.truncate
          message = "[#{name}] Truncated broken object: <#{object_path}>"
          @output.puts(message)
          logger.log(:info, message)

          Dir.entries(object_dirname).sort.each do |path|
            next unless path.start_with?("#{object_basename}.")

            full_path = "#{object_dirname}/#{path}"
            begin
              File.unlink(full_path)
              message =
                "[#{name}] Removed broken object related file: <#{full_path}>"
              @output.puts(message)
              logger.log(:info, message)
            rescue Error => error
              message =
                "[#{name}] Failed to remove broken object related file: " +
                "<#{full_path}>: #{error.class}: #{error.message}"
              @output.puts(message)
              logger.log(:error, message)
              logger.log_error(error)
            end
          end
        end

        def clear_locks
          if @database.locked?
            @database.clear_lock
            logger.log(:info, "Clear locked database: <#{@database.path}>")
          end
          @database.each do |object|
            case object
            when IndexColumn
              # Ignore. It'll be recovered later.
            when Column, Table
              next unless object.locked?
              object.clear_lock
              logger.log(:info,
                         "[#{object.name}] Clear locked object: " +
                         "<#{object.path}>")
            end
          end
        end

        def remove_empty_files
          dirname = File.dirname(@database.path)
          basename = File.basename(@database.path)

          broken_ids = {}
          Dir.entries(dirname).sort.each do |path|
            next unless path.start_with?(basename)

            full_path = "#{dirname}/#{path}"
            next unless File.stat(full_path).size.zero?

            match_data =
              /\A#{Regexp.escape(basename)}\.([\da-fA-F]{7})/.match(path)
            if match_data
              id = Integer(match_data[1], 16)
              name = @database[id]
              if name
                broken_ids[id] = true
                next
              end
            end

            begin
              File.unlink(full_path)
              message = "Removed empty file: <#{full_path}>"
              @output.puts(message)
              logger.log(:info, message)
            rescue Error => error
              message =
                "Failed to remove empty file: <#{full_path}>: " +
                "#{error.class}: #{error.message}"
              logger.log(:error, message)
              logger.log_error(error)
            end
          end

          broken_ids.each_key do |id|
            name = @database[id]
            broken_basename = "%s.%07x" % [basename, id]

            begin
              Object.remove_force(name)
              message =
                "[#{name}] Remove a broken object that has empty file: " +
                "<#{dirname}/#{broken_basename}>"
              @output.puts(message)
              logger.log(:info, message)
            rescue Error => error
              message =
                "[#{name}] Failed to remove a broken object " +
                "that has empty file: <#{dirname}/#{broken_basename}>: " +
                "#{error.class}: #{error.message}"
              logger.log(:error, message)
              logger.log_error(error)
              next
            end

            Dir.entries(dirname).sort.each do |path|
              next unless path.start_with?(broken_basename)

              full_path = "#{dirname}/#{path}"

              begin
                File.unlink(full_path)
                message =
                  "[#{name}] Removed a file for broken object " +
                  "that has empty file: <#{full_path}>"
                @output.puts(message)
                logger.log(:info, message)
              rescue Error => error
                message =
                  "[#{name}] Failed to removed a file for broken object " +
                  "that has empty file: <#{full_path}>: " +
                  "#{error.class}: #{error.message}"
                logger.log(:error, message)
                logger.log_error(error)
              end
            end
          end
        end
      end
    end
  end
end
