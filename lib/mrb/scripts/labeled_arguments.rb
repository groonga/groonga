module Groonga
  class LabeledArguments
    include Enumerable

    def initialize(arguments, prefix_pattern)
      @arguments = arguments
      @pattern = /\A#{prefix_pattern}\[(.+?)\]\.(.+)\z/
    end

    def each(&block)
      labeled_arguments = {}
      @arguments.each do |key, value|
        match_data = @pattern.match(key)
        next if match_data.nil?
        labeled_argument = (labeled_arguments[match_data[1]] ||= {})
        labeled_argument[match_data[2]] = value
      end
      labeled_arguments.each(&block)
    end
  end
end
