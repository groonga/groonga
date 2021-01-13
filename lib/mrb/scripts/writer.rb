module Groonga
  class Writer
    def array(name, n_elements)
      open_array(name, n_elements)
      begin
        yield
      ensure
        close_array
      end
    end

    def map(name, n_elements)
      open_map(name, n_elements)
      begin
        yield
      ensure
        close_map
      end
    end

    def result_set(table, output_columns, n_hits, n_additional_elements=0)
      open_result_set(table, output_columns, n_hits, n_additional_elements)
      begin
        yield
      ensure
        close_result_set
      end
    end
  end
end
