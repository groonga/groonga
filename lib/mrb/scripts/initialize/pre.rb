# We can remove this once
# https://github.com/mattn/mruby-onig-regexp/pull/133 is merged.
class OnigMatchData
  def __group(n)
    self[n]
  end

  def __pre_match
    pre_match
  end

  def __post_match
    post_match
  end

  def __last_group
    last = nil
    captures.each do |capture|
      last = capture if capture
    end
    last
  end
end

require "time"

require "backtrace_entry"

require "operator"

require "loggable"
require "query_loggable"
