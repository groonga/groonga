module Groonga
  module Command
    class Grndb
      def initialize(argv)
        @command, *@arguments = argv
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

        if rest.size < 1
          $stderr.puts("database path is missing")
          return false
        end

        succeeded = false
        database = Groonga::Database.open(rest[0])
        begin
          succeeded = run_action(slop, database)
        ensure
          database.close
        end
        succeeded
      end

      private
      ACTIONS = [:recover, :check]

      def create_slop
        slop = Slop.new
        slop.banner = "Usage: #{File.basename(@command)} [options] DB_PATH"
        slop.on("-h", "--help", "Display this help message.", :tail => true)
        slop.on("--action=",
                "Action to do. Available actions: #{format_actions}",
                :default => ACTIONS.first,
                :as => lambda {|value| action_value(value)})
        slop
      end

      def format_actions
        "[#{ACTIONS.join(", ")}]"
      end

      def action_value(value)
        action = value.to_sym
        case action
        when *ACTIONS
          action
        else
          message = "action must be one of #{format_actions}: #{value.inspect}"
          raise Slop::InvalidArgumentError, message
        end
      end

      def run_action(slop, database)
        case slop[:action]
        when :recover
          database.recover
          true
        when :check
          check(database)
        end
      end

      def check(database)
        all_unlocked = true
        database.each do |object|
          if object.locked?
            # TODO: Report
            all_unlocked = false
          end
        end
        all_unlocked
      end
    end
  end
end
