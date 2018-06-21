module Groonga
  class LocaleOutput
    def puts(*args)
      if args.empty?
        write("\n")
        return nil
      end

      args.each do |arg|
        case arg
        when String
          write(arg)
          write("\n") if arg[-1] != "\n"
        when Array
          next if arg.empty?
          puts(*arg)
        else
          if arg.respond_to?(:to_ary)
            puts(arg.to_ary)
          else
            puts(arg.to_s)
          end
        end
      end
      nil
    end
  end
end
