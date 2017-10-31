module Groonga
  module Sharding
    module KeysParsable
      private
      def parse_keys(raw_keys)
        return [] if raw_keys.nil?

        raw_keys.strip.split(/ *, */)
      end
    end
  end
end
