module Groonga
  class Writer
    def array(name, n_elements)
      open_array(name, n_elements)
      yield
      close_array
    end

    def map(name, n_elements)
      open_map(name, n_elements)
      yield
      close_map
    end
  end
end
