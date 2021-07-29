#!/usr/bin/env ruby

require "msgpack"

unpacker = MessagePack::Unpacker.new(ARGF)
unpacker.each do |object|
  pp object
end
