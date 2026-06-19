module Groonga
  class Accessor
    include Indexable

    private :estimate_size_for_query

    # Estimate the number of matched records for query.
    #
    # @overload estimate_size(:query => query)
    #   @return [Integer] the number of matched records for the query.
    def estimate_size(query: nil, **kw_args)
      if query
        return estimate_size_for_query(query, **kw_args)
      end

      message = "must specify :query: #{kw_args.inspect}"
      raise InvalidArgument, message
    end
  end
end
