module Groonga
  class IndexColumn
    private :estimate_size_for_term_id
    private :estimate_size_for_query
    private :estimate_size_for_lexicon_cursor

    # Estimate the number of matched records for term ID or query.
    #
    # @overload estimate_size(term: term)
    #   @param term [Record] the term as a Record
    #   @return [Integer] the number of matched records for the term ID.
    #
    # @overload estimate_size(term_id: term_id)
    #   @return [Integer] the number of matched records for the term ID.
    #
    # @overload estimate_size(query: query, mode: mode)
    #   @return [Integer] the number of matched records for the query.
    #
    # @overload estimate_size(lexicon_cursor: lexicon_cursor)
    #   @return [Integer] the number of matched records for the lexicon cursor.
    #
    def estimate_size(term: nil,
                      term_id: nil,
                      query: nil,
                      mode: nil,
                      lexicon_cursor: nil)
      if term
        # TODO: Validate lexicon
        return estimate_size_for_term_id(term.id)
      end

      if term_id
        return estimate_size_for_term_id(term_id)
      end

      if query
        return estimate_size_for_query(query, mode: mode)
      end

      if lexicon_cursor
        return estimate_size_for_lexicon_cursor(lexicon_cursor)
      end

      message = "must specify :term_id, :term_id, :query or :lexicon_cursor"
      raise InvalidArgument, message
    end
  end
end
