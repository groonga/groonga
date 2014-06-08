module Kernel
  def puts(*arguments)
    arguments.each do |argument|
      argument = argument.to_s unless argument.is_a?(String)
      print(argument)
      print("\n") unless argument[argument.size] == "\n"
    end
    nil
  end

  def p(*arguments)
    return nil if arguments.empty?

    if arguments.size == 1
      argument = arguments.first
      puts(argument.inspect)
      argument
    else
      arguments.each do |argument|
        puts(argument.inspect)
      end
      arguments
    end
  end
end
