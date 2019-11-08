module Groonga
  class Accessor
    include Indexable

    private :estimate_size_for_query

    # Estimate the number of matched records for query.
    #
    # @overload estimate_size(:query => query)
    #   @return [Integer] the number of matched records for the query.
    def estimate_size(args)
      query = args[:query]
      if query
        return estimate_size_for_query(query, args)
      end

      message =
        "must specify :query: #{args.inspect}"
      raise InvalidArgument, message
    end
  end
end
