module Groonga
  module Sharding
    module KeysParsable
      private
      def parse_keys(raw_keys)
        return [] if raw_keys.nil?

        raw_keys.strip.split(/ *, */)
      end

      def sort_key_name(sort_key)
        sort_key.sub(/\A[-+]/, "")
      end

      def sort_key_descending?(sort_key)
        sort_key.start_with?("-")
      end
    end
  end
end
