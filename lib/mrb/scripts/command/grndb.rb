module Groonga
  module Command
    class Grndb
      def initialize(argv)
        @argv = argv
      end

      def run
        p @argv
      end
    end
  end
end
