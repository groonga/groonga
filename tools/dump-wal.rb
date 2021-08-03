#!/usr/bin/env ruby

require "json"

require "msgpack"

grn_wal_h_path = File.join(__dir__, "..", "lib", "grn_wal.h")
if File.exist?(grn_wal_h_path)
  @events = {}
  @segment_types = {}
  @key_types = {}
  def normalize_enum(enum)
    enum.downcase.gsub("_", "-")
  end
  File.open(grn_wal_h_path) do |grn_wal_h|
    event_index = 0
    segment_type_index = 0
    key_type_index = 0
    grn_wal_h.each_line do |line|
      case line.chomp.chomp(",")
      when /\A  GRN_WAL_EVENT_(.+)\z/
        @events[event_index] = normalize_enum($1)
        event_index += 1
      when /\A  GRN_WAL_SEGMENT_(.+)\z/
        @segment_types[segment_type_index] = normalize_enum($1)
        segment_type_index += 1
      when /\A  GRN_WAL_KEY_(.+)\z/
        @key_types[key_type_index] = normalize_enum($1)
        key_type_index += 1
      end
    end
  end
  def describe(object)
    described = {}
    object.each do |key, value|
      key = @key_types[key] || key
      case key
      when "event"
        value = @events[value] || value
      when "segment-type"
        value = @segment_types[value] || value
      end
      described[key] = value
    end
    described
  end
else
  def describe(object)
    object
  end
end

unpacker = MessagePack::Unpacker.new(ARGF)
unpacker.each do |object|
  puts(describe(object).to_json)
end
