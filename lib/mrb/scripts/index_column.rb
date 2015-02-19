module Groonga
  class IndexColumn
    private :estimate_size_for_term_id
    private :estimate_size_for_query

    # Estimate the number of matched records for term ID or query.
    #
    # @overload estimate_size(:term_id => term_id)
    #   @return [Integer] the number of matched records for the term ID.
    #
    # @overload estimate_size(:query => query)
    #   @return [Integer] the number of matched records for the query.
    #
    def estimate_size(parameters)
      term_id = parameters[:term_id]
      if term_id
        return estimate_size_for_term_id(term_id)
      end

      query = parameters[:query]
      if query
        return estimate_size_for_query(query, parameters)
      end

      message = "must specify :term_id or :query: #{parameters.inspect}"
      raise InvalidArgument, message
    end
  end
end
