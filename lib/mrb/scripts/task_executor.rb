module Groonga
  class TaskExecutor
    # Runs the given block with a dedicated Groonga::ChildContext. The
    # child context is released automatically after the block is
    # finished.
    def execute
      child_context = ChildContext.new
      begin
        yield(child_context)
      ensure
        child_context.release
      end
    end
  end
end
