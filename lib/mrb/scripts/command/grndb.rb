module Groonga
  module Command
    class Grndb
      def initialize(argv)
        @command = argv[0]
        @arguments = argv[1..-1]
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

        database = Groonga::Database.open(rest[0])
        begin
          database.recover
        ensure
          database.close
        end

        true
      end

      private
      def create_slop
        slop = Slop.new
        slop.banner = "Usage: #{File.basename(@command)} [options] DB_PATH"
        slop.on("-h", "--help", "Display this help message.", :tail => true)
        slop
      end
    end
  end
end
