#!/usr/bin/env ruby

require 'English'
require 'ostruct'
require 'optparse'

options = OpenStruct.new

option_parser = OptionParser.new do |parser|
  parser.banner += " LOG1 ..."
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

current_statistics = {}
statistics = []
ARGF.each_line do |line|
  case line
  when /\A(\d{4})-(\d\d)-(\d\d) (\d\d):(\d\d):(\d\d)\.(\d+)\|(.+?)\|>/
    year, month, day, hour, minutes, seconds, micro_seconds =
      $1, $2, $3, $4, $5, $6, $7
    context_id = $8
    command = $POSTMATCH.strip
    start_time = Time.local(year, month, day, hour, minutes, seconds,
                            micro_seconds)
    statistic = Statistic.new(context_id)
    statistic.start_time = start_time
    statistic.command = command
    current_statistics[context_id] = statistic
  when /\A\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d+\|(.+?)\|:(\d+) /
    context_id = $1
    elapsed = $2
    label = $POSTMATCH.strip
    statistic = current_statistics[context_id]
    next if statistic.nil?
    statistic.trace << [elapsed.to_i, label]
  when /\A\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d+\|(.+?)\|<(\d+) rc=(\d+)/
    context_id = $1
    elapsed = $2
    return_code = $3
    statistic = current_statistics.delete(context_id)
    next if statistic.nil?
    statistic.elapsed = elapsed.to_i
    statistic.return_code = return_code.to_i
    statistics << statistic
  end
end

elapsed_sorted_statistics = statistics.sort_by do |statistic|
  -statistic.elapsed
end

elapsed_sorted_statistics[0, 10].each_with_index do |statistic, i|
  puts "%2d) %s" % [i + 1, statistic.label]
  statistic.each_trace_report do |report|
    puts "   #{report}"
  end
  puts
end
