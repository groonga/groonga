module Groonga
  module QueryLoggable
    private
    def query_logger
      Context.instance.query_logger
    end
  end
end
