#!/usr/bin/env ruby

require 'English'
require 'ostruct'
require 'optparse'

options = OpenStruct.new
options.n_entries = 10

option_parser = OptionParser.new do |parser|
  parser.banner += " LOG1 ..."

  parser.on("-n", "--n-entries=N",
            Integer,
            "Show top N entries",
            "(#{options.n_entries})") do |n|
    options.n_entries = n
  end
end

option_parser.parse!(ARGV)

class Statistic
  attr_reader :context_id
  attr_accessor :start_time, :command, :trace, :elapsed, :return_code
  def initialize(context_id)
    @context_id = context_id
    @start_time = nil
    @command = nil
    @trace = []
    @elapsed = nil
    @return_code = 0
  end

  def end_time
    @start_time + nano_seconds_to_seconds(@elapsed)
  end

  def label
    "[%s-%s (%g)](%d): %s" % [format_time(start_time),
                              format_time(end_time),
                              nano_seconds_to_seconds(elapsed),
                              return_code,
                              command]
  end

  def each_trace_report
    previous_elapsed = 0
    @trace.each_with_index do |(trace_elapsed, trace_label), i|
      relative_elapsed = trace_elapsed - previous_elapsed
      previous_elapsed = trace_elapsed
      yield " %2d) %8.8f: %s" % [i + 1,
                                 nano_seconds_to_seconds(relative_elapsed),
                                 trace_label]
    end
  end

  private
  def format_time(time)
    time.strftime("%Y-%m-%d %H:%M:%S.%u")
  end

  def nano_seconds_to_seconds(nano_seconds)
    nano_seconds / 1000.0 / 1000.0 / 1000.0
  end
end

class QueryLogParser
  attr_reader :statistics
  def initialize
    @statistics = []
  end

  def parse(input)
    current_statistics = {}
    input.each_line do |line|
      case line
      when /\A(\d{4})-(\d\d)-(\d\d) (\d\d):(\d\d):(\d\d)\.(\d+)\|(.+?)\|([>:<])/
        year, month, day, hour, minutes, seconds, micro_seconds =
          $1, $2, $3, $4, $5, $6, $7
        context_id = $8
        type = $9
        rest = $POSTMATCH.strip
        time_stamp = Time.local(year, month, day, hour, minutes, seconds,
                                micro_seconds)
        parse_line(current_statistics, time_stamp, context_id, type, rest)
      end
    end
  end

  private
  def parse_line(current_statistics, time_stamp, context_id, type, rest)
    case type
    when ">"
      statistic = Statistic.new(context_id)
      statistic.start_time = time_stamp
      statistic.command = rest
      current_statistics[context_id] = statistic
    when ":"
      return unless /\A(\d+) / =~ rest
      elapsed = $1
      label = $POSTMATCH.strip
      statistic = current_statistics[context_id]
      return if statistic.nil?
      statistic.trace << [elapsed.to_i, label]
    when "<"
      return unless /\A(\d+) rc=(\d+)/ =~ rest
      elapsed = $1
      return_code = $2
      statistic = current_statistics.delete(context_id)
      return if statistic.nil?
      statistic.elapsed = elapsed.to_i
      statistic.return_code = return_code.to_i
      @statistics << statistic
    end
  end
end

class QueryLogReporter
  include Enumerable

  attr_accessor :n_entries
  def initialize(statistics)
    @statistics = statistics
    @order = :elapsed
    @n_entries = 10
    @sorted_statistics = nil
  end

  def order=(order)
    return if @order == order
    @order = order
    @sorted_statistics = nil
  end

  def sorted_statistics
    @sorted_statistics ||= @statistics.sort_by(&sorter)
  end

  def each
    sorted_statistics.each_with_index do |statistic, i|
      break if i >= @n_entries
      yield statistic
    end
  end

  private
  def sorter
    case @order
    when :elapsed
      lambda do |statistic|
        -statistic.elapsed
      end
    else
      lambda do |statistic|
        statistic.start_time
      end
    end
  end
end

class ConsoleQueryLogReporter < QueryLogReporter
  def report
    digit = Math.log10(n_entries).truncate + 1
    each_with_index do |statistic, i|
      puts "%*d) %s" % [digit, i + 1, statistic.label]
      statistic.each_trace_report do |report|
        puts "   #{report}"
      end
      puts
    end
  end
end

parser = QueryLogParser.new
parser.parse(ARGF)

reporter = ConsoleQueryLogReporter.new(parser.statistics)
reporter.n_entries = options.n_entries
reporter.report
