#!/usr/bin/env ruby

class Memory < Struct.new(:size, :file, :line, :function)
  def location
    "#{file}:#{line}"
  end
end

class LocationGroup
  attr_reader :location
  attr_reader :memories
  def initialize(location)
    @location = location
    @memories = []
  end

  def add(memory)
    @memories << memory
  end

  def total_size
    @memories.inject(0) do |sum, memory|
      sum + memory.size
    end
  end

  def average_size
    total_size / @memories.size.to_f
  end
end

class Statistics
  def initialize
    @location_groups = {}
  end

  def add(memory)
    group = location_group(memory.location)
    group.add(memory)
  end

  def sort_by_size
    @location_groups.values.sort_by do |group|
      group.total_size
    end
  end

  private
  def location_group(location)
    @location_groups[location] ||= LocationGroup.new(location)
  end
end

statistics = Statistics.new

ARGF.each_line do |line|
  case line
  when /\Aaddress\[\d+\]\[not-freed\]:\s
          (?:0x)?[\da-fA-F]+\((\d+)\):\s
          (.+?):(\d+):\s(\S+)/x
    size = $1.to_i
    file = $2
    line = $3.to_i
    function = $4.strip
    memory = Memory.new(size, file, line, function)
    statistics.add(memory)
  end
end

def format_size(size)
  if size < 1024
    "#{size}B"
  elsif size < (1024 * 1024)
    "%.3fKiB" % (size / 1024.0)
  elsif size < (1024 * 1024 * 1024)
    "%.3fMiB" % (size / 1024.0 / 1024.0)
  elsif size < (1024 * 1024 * 1024 * 1024)
    "%.3fGiB" % (size / 1024.0 / 1024.0 / 1024.0)
  else
    "#{size}B"
  end
end

statistics.sort_by_size.reverse[0, 10].each do |group|
  puts("%10s(%10s): %s(%d)" % [
         format_size(group.total_size),
         format_size(group.average_size),
         group.location,
         group.memories.size,
       ])
end
