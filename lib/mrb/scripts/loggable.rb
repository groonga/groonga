module Groonga
  module Loggable
    private
    def logger
      Context.instance.logger
    end
  end
end
