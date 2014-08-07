module Groonga
  class Expression
    def build_scan_info(op, size)
      Context.instance.guard do
        builder = ScanInfoBuilder.new(self, op, size)
        builder.build
      end
    end
  end
end
