#!/usr/bin/env ruby
#
# Copyright(C) 2025  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

require "test-unit"

module ParsedJSON
  UINT8_SIZE = 1
  UINT16_SIZE = 2
  UINT32_SIZE = 4
  UINT64_SIZE = 8

  # Format:
  #
  # +------------------+-------+---------+----------------+
  # | ROOT_TAG(OBJECT) | FLAGS | BUFFERS | BUFFER_OFFSETS |
  # +------------------+-------+---------+----------------+
  #
  # +-----------------+-------+---------+----------------+
  # | ROOT_TAG(ARRAY) | FLAGS | BUFFERS | BUFFER_OFFSETS |
  # +-----------------+-------+---------+----------------+
  #
  # +------------------+-------+
  # | ROOT_TAG(STRING) | VALUE |
  # +------------------+-------+
  #
  # +-------------------+-------+
  # | ROOT_TAG(INTEGER) | VALUE |
  # +-------------------+-------+
  #
  # +------------------+-------+
  # | ROOT_TAG(DOUBLE) | VALUE |
  # +------------------+-------+
  #
  # +--------------------+
  # | ROOT_TAG(CONSTANT) |
  # +--------------------+
  #
  # ROOT_TAG: 8 bits: MMMMETTT: TAG - the Data part
  #   See TAG for MMMMETTT details.
  #
  #   If TTT is OBJECT or ARRAY, from 2nd byte to 4th byte is FLAGS
  #   (big endian). BUFFERS has data and BUFFERS_OFFSETS shows offset
  #   of each buffer in BUFFERS. Index of OBJECT/ARRAY is always 0.
  #
  #   If TTT is STRING, from 2 byte to the end is the root string. The
  #   length information isn't provided. User must know the total
  #   parsed JSON data size.
  #
  #   If TTT is INTEGER:
  #     If MMMM is 1, from 2 byte to 3 byte is the root int8 value.
  #     If MMMM is 2, from 2 byte to 4 byte is the root int16 value.
  #     If MMMM is 4, from 2 byte to 5 byte is the root int32 value.
  #     If MMMM is 8, from 2 byte to 9 byte is the root int64 value.
  #
  #   If TTT is DOUBLE:
  #     If E is 1, MMMM represents the value. See TAG for the details.
  #     If E is 0, from 2 byte to 9 byte is the root double value.
  #
  #   If TTT is CONSTANT, MMMM represents the value. See TAG for the
  #   details.
  #
  # FLAGS: 3 bytes (big endian): Flags
  #   * 0b00000000_00000000_00000001:
  #     * OBJECT_VALUE16: OBJECT 16 bits values buffer is used
  #   * 0b00000000_00000000_00000010:
  #     * OBJECT_VALUE32: OBJECT 32 bits values buffer is used
  #   * 0b00000000_00000000_00000100:
  #     * OBJECT_POSITION8: OBJECT 8 bits positions buffer is used
  #   * 0b00000000_00000000_00001000:
  #     * OBJECT_POSITION16: OBJECT 16 bits positions buffer is used
  #   * 0b00000000_00000000_00010000:
  #     * OBJECT_POSITION32: OBJECT 32 bits positions buffer is used
  #   * 0b00000000_00000000_00100000:
  #     * ARRAY_VALUE16: ARRAY 16 bits values buffer is used
  #   * 0b00000000_00000000_01000000:
  #     * ARRAY_VALUE32: ARRAY 32 bits values buffer is used
  #   * 0b00000000_00000000_10000000:
  #     * ARRAY_POSITION8: ARRAY 8 bits positions buffer is used
  #   * 0b00000000_00000001_00000000:
  #     * ARRAY_POSITION16: ARRAY 16 bits positions buffer is used
  #   * 0b00000000_00000010_00000000:
  #     * ARRAY_POSITION32: ARRAY 32 bits positions buffer is used
  #   * 0b00000000_00000100_00000000:
  #     * STRING_VALUE: STRING values buffer is used
  #   * 0b00000000_00001000_00000000:
  #     * STRING_POSITION8: STRING 8 bits positions buffer is used
  #   * 0b00000000_00010000_00000000:
  #     * STRING_POSITION16: STRING 16 bits positions buffer is used
  #   * 0b00000000_00100000_00000000:
  #     * STRING_POSITION32: STRING 32 bits positions buffer is used
  #   * 0b00000000_01000000_00000000:
  #     * INT16: INT16 values buffer is used
  #   * 0b00000000_10000010_00000000:
  #     * INT32: INT32 values buffer is used
  #   * 0b00000001_00000000_00000000:
  #     * INT64: INT64 values buffer is used
  #   * 0b00000010_00000000_00000000:
  #     * DOUBLE: DOUBLE values buffer is used
  #   * 0b00000100_00000000_00000000:
  #     * OFFSET32: BUFFER_OFFSETS uses 32 bits
  #
  # BUFFERS: It contains the following buffers. They can be accessed
  #          by offsets in the BUFFER_OFFSETS.
  #   * TAG 32 bits buffer
  #   * OBJECT 16 bits values buffer
  #   * OBJECT 32 bits values buffer
  #   * OBJECT 8 bits positions buffer
  #   * OBJECT 16 bits positions buffer
  #   * OBJECT 32 bits positions buffer
  #   * ARRAY 16 bits values buffer
  #   * ARRAY 32 bits values buffer
  #   * ARRAY 8 bits positions buffer
  #   * ARRAY 16 bits positions buffer
  #   * ARRAY 32 bits positions buffer
  #   * STRING values buffer
  #   * STRING 8 bits positions buffer
  #   * STRING 16 bits positions buffer
  #   * STRING 32 bits positions buffer
  #   * INTEGER(int16) values buffer
  #   * INTEGER(int32) values buffer
  #   * INTEGER(int64) values buffer
  #   * DOUBLE values buffer
  #
  # BUFFER_OFFSETS: 32 bits if FLAGS has OFFSET32, 16 bits otherwise
  #   It contains the following offsets in this order:
  #     * If FLAGS has TAG32:
  #       * TAG 32 bits buffer offset
  #     * If FLAGS has OBJECT_VALUE16:
  #       * OBJECT 16 bits values buffer offset
  #     * If FLAGS has OBJECT_VALUE32:
  #       * OBJECT 32 bits values buffer offset
  #     * If FLAGS has OBJECT_POSITION8:
  #       * OBJECT 8 bits positions buffer offset
  #     * If FLAGS has OBJECT_POSITION16:
  #       * OBJECT 16 bits positions buffer offset
  #     * If FLAGS has OBJECT_POSITION32:
  #       * OBJECT 32 bits positions buffer offset
  #     * If FLAGS has ARRAY_VALUE16:
  #       * ARRAY 16 bits values buffer offset
  #     * If FLAGS has ARRAY_VALUE32:
  #       * ARRAY 32 bits values buffer offset
  #     * If FLAGS has ARRAY_POSITION8:
  #       * ARRAY 8 bits positions buffer offset
  #     * If FLAGS has ARRAY_POSITION16:
  #       * ARRAY 16 bits positions buffer offset
  #     * If FLAGS has ARRAY_POSITION32:
  #       * ARRAY 32 bits positions buffer offset
  #     * If FLAGS has STRING_VALUE:
  #       * STRING values buffer offset
  #     * If FLAGS has STRING_POSITION8:
  #       * STRING 8 bits positions buffer offset
  #     * If FLAGS has STRING_POSITION16:
  #       * STRING 16 bits positions buffer offset
  #     * If FLAGS has STRING_POSITION32:
  #       * STRING 32 bits positions buffer offset
  #     * If FLAGS has INT16:
  #       * INTEGER(int16) values buffer offset
  #     * If FLAGS has INT32:
  #       * INTEGER(int32) values buffer offset
  #     * If FLAGS has INT64:
  #       * INTEGER(int64) values buffer offset
  #     * If FLAGS has DOUBLE:
  #       * DOUBLE values buffer offset

  ROOT_TAG_SIZE = UINT8_SIZE

  module Flags
    OBJECT_VALUE16    = 0b00000000_00000000_00000001
    OBJECT_VALUE32    = 0b00000000_00000000_00000010
    OBJECT_POSITION8  = 0b00000000_00000000_00000100
    OBJECT_POSITION16 = 0b00000000_00000000_00001000
    OBJECT_POSITION32 = 0b00000000_00000000_00010000
    ARRAY_VALUE16     = 0b00000000_00000000_00100000
    ARRAY_VALUE32     = 0b00000000_00000000_01000000
    ARRAY_POSITION8   = 0b00000000_00000000_10000000
    ARRAY_POSITION16  = 0b00000000_00000001_00000000
    ARRAY_POSITION32  = 0b00000000_00000010_00000000
    STRING_VALUE      = 0b00000000_00000100_00000000
    STRING_POSITION8  = 0b00000000_00001000_00000000
    STRING_POSITION16 = 0b00000000_00010000_00000000
    STRING_POSITION32 = 0b00000000_00100000_00000000
    INT16             = 0b00000000_01000000_00000000
    INT32             = 0b00000000_10000000_00000000
    INT64             = 0b00000001_00000000_00000000
    DOUBLE            = 0b00000010_00000000_00000000
    OFFSET32          = 0b00000100_00000000_00000000
  end

  module Type
    OBJECT   = 0
    ARRAY    = 1
    STRING   = 2
    INTEGER  = 3
    DOUBLE   = 4
    CONSTANT = 5

    class << self
      def resolve_value(value)
        constants.find {|name| const_get(name) == value}
      end
    end
  end

  module Metadata
    module Double
      NAN               = 0b0000
      POSITIVE_INFINITY = 0b0001
      NEGATIVE_INFINITY = 0b0010
      POSITIVE_ZERO     = 0b0100
      NEGATIVE_ZERO     = 0b1000

      class << self
        def resolve_value(value)
          constants.find {|name| const_get(name) == value}
        end
      end
    end

    module Constant
      TRUE  = 0b0000
      FALSE = 0b0001
      NULL  = 0b0010

      class << self
        def resolve_value(value)
          constants.find {|name| const_get(name) == value}
        end
      end
    end
  end

  # TAG: 16 bits: DDDDDDDD MMMMETTT
  # TAG: 32 bits: DDDDDDDD DDDDDDD DDDDDDDD MMMMETTT
  #
  # TTT: Type
  # E: Embedded or not (1: embedded, 0: not embedded)
  # MMMM: Metadata
  # DDDDDDDD: Data:
  # DDDDDDDD DDDDDDDD DDDDDDDD: Data:
  #   OBJECT:
  #     Index in objects.
  #     E is always 0.
  #     MMMM is always 0000.
  #   ARRAY:
  #     Index in arrays.
  #     E is always 0.
  #     MMMM is always 0000.
  #   STRING:
  #     If E is 1:
  #       0-3 bytes string itself.
  #       MMMM shows the number of bytes.
  #     Else:
  #       Index in strings.
  #       MMMM is 0000.
  #   INTEGER:
  #     MMMM shows the byte width.
  #     If TAG is 16 bits:
  #       If E is 1:
  #         -(2^7) - (2^7-1): Itself. We can embed only int32 values.
  #       Else:
  #         -(2^15) - (2^15-1): Index in int16 values.
  #         -(2^31) - (2^31-1): Index in int32 values.
  #         -(2^63) - (2^63-1): Index in int64 values.
  #     Else:
  #       If E is 1:
  #         -(2^15) - (2^27-1): Itself. We can't embed int32 negative numbers.
  #       Else:
  #         -(2^31) - (2^31-1): Index in int32 values.
  #         -(2^63) - (2^63-1): Index in int64 values.
  #   DOUBLE:
  #     If E is 1: (FYI: JSON doesn't permit Infinity and NaN...)
  #       NaN:       MMMM is 0000
  #       +Infinity: MMMM is 0001
  #       -Infinity: MMMM is 0010
  #       +0.0:      MMMM is 0100
  #       -0.0:      MMMM is 1000
  #     Else:
  #       Index in double values.
  #   CONSTANT: Always 0. E is always 1.
  #     true:  MMMM is 0000
  #     false: MMMM is 0001
  #     null:  MMMM is 0010

  def pack_tag(type, is_embedded, metadata, data)
    tag = type
    tag |= (1 << 3) if is_embedded
    tag |= (metadata << 4)
    tag |= (data << 8)
    tag
  end

  def unpack_tag(tag)
    type = (tag & (0b111))
    is_embedded = (tag & (0b1000)) == 0b1000
    metadata = ((tag >> 4) & 0b00001111)
    data = (tag >> 8)
    [type, is_embedded, metadata, data]
  end

  # The following types use external buffers:
  #
  #   * OBJECT
  #   * ARRAY
  #   * STRING
  #   * INTEGER(int16)
  #   * INTEGER(int32)
  #   * INTEGER(int64)
  #   * DOUBLE
  #
  # OBJECT:
  #   Written in breadth-first order for nested objects.
  #
  #   OBJECT uses 2 or 3 buffers:
  #     * 16 bits values buffer:
  #         members.each do |key, value|
  #           buffer << key.tag
  #           buffer << value.tag
  #         end
  #     * Optional 32 bits values buffer. 32 bits values buffer must exist
  #       after 16 bits values buffer:
  #         members.each do |key, value|
  #           buffer << key.tag
  #           buffer << value.tag
  #         end
  #     * Positions buffers:
  #       * uint8_t (N: 0..255)
  #       * uint16_t (N: 256..65535)
  #       * uint32_t (N: 65536..)
  #       * [
  #           N_MEMBERS_OF_1ST_OBJECT,
  #           N_MEMBERS_OF_1ST_OBJECT + N_MEMBERS_OF_2ND_OBJECT,
  #           ...,
  #         ]
  #       * positions[N - 1] - positions[N]:
  #         The number of members of the Nth object.
  #         If N == 0, positions[N - 1] is processed as 0.
  #       * values[positions[N]..(positions[N - 1] - positions[N])]:
  #         The members of the Nth object.
  #
  # ARRAY
  #   Written in breadth-first order for nested arrays.
  #
  #   ARRAY uses 2 or 3buffers:
  #     * 16 bits values buffer:
  #         elements.each do |element|
  #           buffer << element.tag
  #         end
  #     * Optional 32 bits values buffer. 32 bits values buffer must exist
  #       after 16 bits values buffer:
  #         elements.each do |element|
  #           buffer << element.tag
  #         end
  #     * Positions buffer:
  #       * uint8_t (N: 0..255)
  #       * uint16_t (N: 256..65535)
  #       * uint32_t (N: 65536..)
  #       * [
  #           N_ELEMENTS_OF_1ST_ARRAY,
  #           N_ELEMENTS_OF_1ST_ARRAY + N_ELEMENTS_OF_2ND_ARRAY,
  #           ...,
  #         ]
  #       * positions[N - 1] - positions[N]:
  #         The number of elements of the Nth array.
  #         If N == 0, positions[N - 1] is processed as 0.
  #       * values[positions[N]..(positions[N - 1] - positions[N])]:
  #         The elements of the Nth array.
  #
  # STRING
  #   STRING uses 2 buffers:
  #     * Values buffer: String itself.
  #     * Positions buffers:
  #       * uint8_t (N: 0..255)
  #       * uint16_t (N: 256..65535)
  #       * uint32_t (N: 65536..)
  #       * [
  #           BYTE_SIZE_OF_1ST_STRING,
  #           BYTE_SIZE_OF_1ST_STRING + BYTE_SIZE_OF_2ND_STRING,
  #           ...,
  #         ]
  #       * positions[N - 1] - positions[N]:
  #         The byte size of the Nth string.
  #         If N == 0, positions[N - 1] is processed as 0.
  #       * values[positions[N]..(positions[N - 1] - positions[N])]:
  #         The Nth string.
  #
  # INTEGER(int16)
  #   INTEGER(int16) uses 1 buffer:
  #     * Values buffer: int16 itself in the native endian.
  #
  # INTEGER(int32)
  #   INTEGER(int32) uses 1 buffer:
  #     * Values buffer: int32 itself in the native endian.
  #
  # INTEGER(int64)
  #   INTEGER(int64) uses 1 buffer:
  #     * Values buffer: int64 itself in the native endian.
  #
  # DOUBLE
  #   DOUBLE uses 1 buffer:
  #     * Values buffer: double itself in the native endian.
end

class ParsedJSONWriter
  include ParsedJSON

  class TagWriter
    include ParsedJSON

    attr_reader :buffer
    attr_reader :size16
    attr_reader :size32
    def initialize(name, buffer)
      @name = name
      @buffer = buffer
      @size16 = 0
      @size32 = 0
    end

    def write(type, is_embedded, metadata, data)
      tag = pack_tag(type, is_embedded, metadata, data)
      offset = @buffer.bytesize
      if @size32 > 0 or data > 255
        @buffer << [tag].pack("L")
        @size32 += UINT32_SIZE
      else
        @buffer << [tag].pack("S")
        @size16 += UINT16_SIZE
      end
    end
  end

  class RootTagWriter
    include ParsedJSON

    def initialize(buffer)
      @buffer = buffer
    end

    def write(type, is_embedded, metadata, data)
      unless data.zero?
        raise "Root tag's data must be zero: #{data.inspect}"
      end
      root_tag = pack_tag(type, is_embedded, metadata, 0)
      @buffer << [root_tag].pack("C")
    end
  end

  class PositionWriter
    include ParsedJSON

    attr_reader :buffer
    attr_reader :size8
    attr_reader :size16
    attr_reader :size32
    def initialize(name, buffer)
      @name = name
      @buffer = buffer
      @i = 0
      @last_position = 0
      @size8 = 0
      @size16 = 0
      @size32 = 0
    end

    def write(size)
      i = @i
      @i += 1
      last_position = @last_position
      position = @last_position + size
      if @size32 > 0 or position > 65535
        @buffer << [position].pack("L")
        @size32 += UINT32_SIZE
      elsif @size16 > 0 or position > 255
        @buffer << [position].pack("S")
        @size16 += UINT16_SIZE
      else
        @buffer << [position].pack("C")
        @size8 += UINT8_SIZE
      end
      @last_position = position
      i
    end
  end

  def initialize(output, target)
    @output = output
    @target = target
    @root_tag_writer = RootTagWriter.new(@output)
    @object_values = +"".b
    @object_values_writer = TagWriter.new(:object, @object_values)
    @object_positions = +"".b
    @object_positions_writer = PositionWriter.new(:object, @object_positions)
    @array_values = +"".b
    @array_values_writer = TagWriter.new(:array, @array_values)
    @array_positions = +"".b
    @array_positions_writer = PositionWriter.new(:array, @array_positions)
    @string_values = +"".b
    @string_positions = +"".b
    @string_positions_writer = PositionWriter.new(:string, @string_positions)
    @int16_values = +"".b
    @int32_values = +"".b
    @int64_values = +"".b
    @double_values = +"".b
  end

  def write
    write_value(@root_tag_writer, @target, true)
  end

  private
  def write_value(tag_writer, value, is_root)
    case value
    when Hash # object
      write_container(tag_writer, value, is_root)
    when Array
      write_container(tag_writer, value, is_root)
    when String
      write_string(tag_writer, value, is_root)
    when Integer
      write_integer(tag_writer, value, is_root)
    when Float
      write_double(tag_writer, value, is_root)
    when true
      tag_writer.write(Type::CONSTANT, true, Metadata::Constant::TRUE, 0)
    when false
      tag_writer.write(Type::CONSTANT, true, Metadata::Constant::FALSE, 0)
    when nil
      tag_writer.write(Type::CONSTANT, true, Metadata::Constant::NULL, 0)
    else
      raise "Unknown value: #{value.inspect}"
    end
  end

  def write_container(tag_writer, target, is_root)
    targets = [target]
    tag_writers = [tag_writer]
    until targets.empty?
      target = targets.shift
      tag_writer = tag_writers.shift
      case target
      when Hash
        index = @object_positions_writer.write(target.size)
        tag_writer.write(Type::OBJECT, false, 0, index)
        target.each do |name, value|
          targets << name
          tag_writers << @object_values_writer
          targets << value
          tag_writers << @object_values_writer
        end
      when Array
        index = @array_positions_writer.write(target.size)
        tag_writer.write(Type::ARRAY, false, 0, index)
        target.each do |value|
          targets << value
          tag_writers << @array_values_writer
        end
      else
        write_value(tag_writer, target, false)
      end
    end
    if is_root
      write_footer
    end
  end

  def append_tag_buffer_offsets(flags,
                                offset,
                                buffer_offsets,
                                tag_writer,
                                flag16,
                                flag32)
    if tag_writer.size16 > 0
      flags |= flag16
      buffer_offsets << offset
      offset += tag_writer.size16
    end
    if tag_writer.size32 > 0
      flags |= flag32
      buffer_offsets << offset
      offset += tag_writer.size32
    end
    [flags, offset]
  end

  def append_position_buffer_offsets(flags,
                                     offset,
                                     buffer_offsets,
                                     position_writer,
                                     flag8,
                                     flag16,
                                     flag32)
    if position_writer.size8 > 0
      flags |= flag8
      buffer_offsets << offset
      offset += position_writer.size8
    end
    if position_writer.size16 > 0
      flags |= flag16
      buffer_offsets << offset
      offset += position_writer.size16
    end
    if position_writer.size32 > 0
      flags |= flag32
      buffer_offsets << offset
      offset += position_writer.size32
    end
    [flags, offset]
  end

  def write_footer
    flags = 0
    offset = UINT32_SIZE
    outputs = []
    buffer_offsets = []

    unless @object_values.empty?
      outputs << @object_values
      flags, offset = append_tag_buffer_offsets(flags,
                                                offset,
                                                buffer_offsets,
                                                @object_values_writer,
                                                Flags::OBJECT_VALUE16,
                                                Flags::OBJECT_VALUE32)
    end
    unless @object_positions.empty?
      outputs << @object_positions
      flags, offset = append_position_buffer_offsets(flags,
                                                     offset,
                                                     buffer_offsets,
                                                     @object_positions_writer,
                                                     Flags::OBJECT_POSITION8,
                                                     Flags::OBJECT_POSITION16,
                                                     Flags::OBJECT_POSITION32)
    end
    unless @array_values.empty?
      outputs << @array_values
      flags, offset = append_tag_buffer_offsets(flags,
                                                offset,
                                                buffer_offsets,
                                                @array_values_writer,
                                                Flags::ARRAY_VALUE16,
                                                Flags::ARRAY_VALUE32)
    end
    unless @array_positions.empty?
      outputs << @array_positions
      flags, offset = append_position_buffer_offsets(flags,
                                                     offset,
                                                     buffer_offsets,
                                                     @array_positions_writer,
                                                     Flags::ARRAY_POSITION8,
                                                     Flags::ARRAY_POSITION16,
                                                     Flags::ARRAY_POSITION32)
    end
    unless @string_values.empty?
      flags |= Flags::STRING_VALUE
      outputs << @string_values
      buffer_offsets << offset
      offset += @string_values.bytesize
      outputs << @string_positions
      flags, offset = append_position_buffer_offsets(flags,
                                                     offset,
                                                     buffer_offsets,
                                                     @string_positions_writer,
                                                     Flags::STRING_POSITION8,
                                                     Flags::STRING_POSITION16,
                                                     Flags::STRING_POSITION32)
    end
    unless @int16_values.empty?
      flags |= Flags::INT16
      outputs << @int16_values
      buffer_offsets << offset
      offset += @int16_values.bytesize
    end
    unless @int32_values.empty?
      flags |= Flags::INT32
      outputs << @int32_values
      buffer_offsets << offset
      offset += @int32_values.bytesize
    end
    unless @int64_values.empty?
      flags |= Flags::INT64
      outputs << @int64_values
      buffer_offsets << offset
      offset += @int64_values.bytesize
    end
    unless @double_values.empty?
      flags |= Flags::DOUBLE
      outputs << @double_values
      buffer_offsets << offset
      offset += @double_values.bytesize
    end
    if not buffer_offsets.empty? and buffer_offsets.last > 65535
      flags |= Flags::OFFSET32
      outputs << buffer_offsets.pack("L*")
    else
      outputs << buffer_offsets.pack("S*")
    end
    @output << [flags >> 16, (flags >> 8) & (0xff), flags & 0xff].pack("C3")
    outputs.each do |output|
      @output << output
    end
  end

  def write_string(tag_writer, value, is_root)
    if is_root
      is_embedded = false
    else
      if tag_writer.size32 > 0
        max_embeddable_size = 3
      else
        max_embeddable_size = 1
      end
      is_embeddable = value.bytesize <= max_embeddable_size
    end
    if is_embeddable
      data = 0
      value.unpack("C*").each.with_index do |character, i|
        data |= character << (8 * i)
      end
      tag_writer.write(Type::STRING, true, value.bytesize, data)
    else
      if is_root
        output = @output
        index = 0
      else
        output = @string_values
        index = @string_positions_writer.write(value.bytesize)
      end
      tag_writer.write(Type::STRING, false, 0, index)
      output.append_as_bytes(value)
    end
  end

  def detect_integer_n_bytes(value)
    if -(2 ** 7) <= value and value <= (2 ** 7 - 1)
      1
    elsif -(2 ** 15) <= value and value <= (2 ** 15 - 1)
      2
    elsif -(2 ** 31) <= value and value <= (2 ** 31 - 1)
      3
    elsif -(2 ** 63) <= value and value <= (2 ** 63 - 1)
      4
    else
      nil
    end
  end

  def write_integer(tag_writer, value, is_root)
    n_bytes = detect_integer_n_bytes(value)
    if n_bytes.nil?
      write_double(tag_writer, value.to_f, is_root)
      return
    end

    if is_root
      is_embedded = false
    else
      is_tag32 = (tag_writer.size32 > 0)
      if is_tag32
        is_embeddable = (-(2 ** 15) <= value and value <= (2 ** 23 - 1))
      else
        is_embeddable = (n_bytes == 1)
      end
    end
    if is_embeddable
      if not is_tag32 and value < 0
        value += 256
      end
      tag_writer.write(Type::INTEGER, true, n_bytes, value)
    else
      if is_root
        output = @output
        offset = 0
      else
        if n_bytes == 2
          output = @int16_values
        elsif n_bytes == 3
          output = @int32_values
        else
          output = @int64_values
        end
        offset = output.bytesize
      end
      tag_writer.write(Type::INTEGER, false, n_bytes, offset)
      if n_bytes == 2
        output << [value].pack("s")
      elsif n_bytes == 2
        output << [value].pack("l")
      else
        output << [value].pack("q")
      end
    end
  end

  def write_double(tag_writer, value, is_root)
    if value.nan?
      tag_writer.write(Type::DOUBLE, true, Metadata::Double::NAN, 0)
    elsif value.infinite?
      if value.positive?
        tag_writer.write(Type::DOUBLE,
                         true,
                         Metadata::Double::POSITIVE_INFINITY,
                         0)
      else
        tag_writer.write(Type::DOUBLE,
                         true,
                         Metadata::Double::NEGATIVE_INFINITY,
                         0)
      end
    elsif value.zero?
      if value.positive?
        tag_writer.write(Type::DOUBLE,
                         true,
                         Metadata::Double::POSITIVE_ZERO,
                         0)
      else
        tag_writer.write(Type::DOUBLE,
                         true,
                         Metadata::Double::NEGATIVE_ZERO,
                         0)
      end
    else
      if is_root
        output = @output
        offset = 0
      else
        output = @double_values
        offset = output.bytesize
      end
      tag_writer.write(Type::DOUBLE, false, 0, offset)
      output << [value].pack("d")
    end
  end
end

class ParsedJSONReader
  include ParsedJSON

  class VariableSizeTagResolver
    include ParsedJSON

    def initialize(input, offset16, n16, offset32, n32)
      @input = input
      @offset16 = offset16
      @n16 = n16
      @offset32 = offset32
      @n32 = n32
    end

    def resolve(i)
      if i < @n16
        offset = @offset16 + i * UINT16_SIZE
        @input.unpack1("S", offset: offset)
      else
        offset = @offset32 + (i - @n16) * UINT32_SIZE
        @input.unpack1("L", offset: offset)
      end
    end
  end

  class VariableSizePositionResolver
    include ParsedJSON

    def initialize(input, offset8, n8, offset16, n16, offset32, n32)
      @input = input
      @offset8 = offset8
      @n8 = n8
      @offset16 = offset16
      @n16 = n16
      @offset32 = offset32
      @n32 = n32
    end

    def resolve(i)
      if i < @n8
        offset = @offset8 + (i * UINT8_SIZE)
        if i == 0
          start = 0
          next_start = @input.unpack1("C", offset: offset)
        else
          previous_offset = offset - UINT8_SIZE
          start, next_start = @input.unpack("C2", offset: previous_offset)
        end
      elsif i < @n8 + @n16
        offset = @offset16 + ((i - @n8) * UINT16_SIZE)
        if i == 0
          start = 0
          next_start = @input.unpack1("S", offset: offset)
        elsif i == @n8
          last_offset8_offset = @offset8 + (@n8 - 1) * UINT8_SIZE
          start = @input.unpack1("C", offset: last_offset8_offset)
          next_start = @input.unpack1("S", offset: offset)
        else
          previous_offset = offset - UINT16_SIZE
          start, next_start = @input.unpack("S2", offset: previous_offset)
        end
      else
        offset = @offset32 + ((i - @n8 - @n16) * UINT32_SIZE)
        if i == 0
          start = 0
          next_start = @input.unpack1("L", offset: offset)
        elsif i == @n8 + @n16
          if @n16 == 0
            if @n8 == 0
              start = 0
            else
              last_offset8_offset = @offset8 + ((@n8 - 1) * UINT8_SIZE)
              start = @input.unpack1("C", offset: last_offset8_offset)
            end
          else
            last_offset16_offset = @offset16 + ((@n16 - 1) * UINT16_SIZE)
            start = @input.unpack1("S", offset: last_offset16_offset)
          end
          next_start = @input.unpack1("L", offset: offset)
        else
          previous_offset = offset - UINT32_SIZE
          start, next_start = @input.unpack("L2", offset: previous_offset)
        end
      end
      [start, next_start]
    end
  end

  def initialize(input)
    @input = input
  end

  def read
    root_tag = @input.unpack1("C")
    read_value(root_tag, true)
  end

  private
  def read_value(tag, is_root)
    type, is_embedded, metadata, data = unpack_tag(tag)
    if type == Type::CONSTANT
      case metadata
      when Metadata::Constant::TRUE
        return true
      when Metadata::Constant::FALSE
        return false
      when Metadata::Constant::NULL
        return nil
      else
        raise "Unknown constant metadata: #{metadata.inspect}"
      end
    end

    if is_root
      case type
      when Type::OBJECT, Type::ARRAY
        read_footer
      end
    end

    case type
    when Type::OBJECT
      read_object(data, is_root)
    when Type::ARRAY
      read_array(data, is_root)
    when Type::STRING
      read_string(is_embedded, metadata, data, is_root)
    when Type::INTEGER
      read_integer(is_embedded, metadata, data, is_root)
    when Type::DOUBLE
      read_double(is_embedded, metadata, data, is_root)
    else
      raise "Unknown type: #{type.inspect}"
    end
  end

  def read_footer
    @flags = 0
    @input.unpack("C3", offset: UINT8_SIZE).each_with_index do |flag, i|
      @flags |= (flag << (8 * (2 - i)))
    end

    n_buffer_offsets = 0
    if flagged?(Flags::OBJECT_VALUE16)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::OBJECT_VALUE32)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::OBJECT_POSITION8)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::OBJECT_POSITION16)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::OBJECT_POSITION32)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::ARRAY_VALUE16)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::ARRAY_VALUE32)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::ARRAY_POSITION8)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::ARRAY_POSITION16)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::ARRAY_POSITION32)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::STRING_VALUE)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::STRING_POSITION8)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::STRING_POSITION16)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::STRING_POSITION32)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::INT16)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::INT32)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::INT64)
      n_buffer_offsets += 1
    end
    if flagged?(Flags::DOUBLE)
      n_buffer_offsets += 1
    end

    if flagged?(Flags::OFFSET32)
      buffer_offsets_offset =
        @input.bytesize - (UINT32_SIZE * n_buffer_offsets)
      buffer_offsets = @input.unpack("L#{n_buffer_offsets}",
                                     offset: buffer_offsets_offset)
    else
      buffer_offsets_offset =
        @input.bytesize - (UINT16_SIZE * n_buffer_offsets)
      buffer_offsets = @input.unpack("S#{n_buffer_offsets}",
                                     offset: buffer_offsets_offset)
    end
    buffer_offsets << @input.bytesize
    i = 0
    i, @object_tag_resolver =
      create_variable_size_tag_resolver(Flags::OBJECT_VALUE16,
                                        Flags::OBJECT_VALUE32,
                                        buffer_offsets,
                                        i)
    i, @object_position_resolver =
      create_variable_size_position_resolver(Flags::OBJECT_POSITION8,
                                             Flags::OBJECT_POSITION16,
                                             Flags::OBJECT_POSITION32,
                                             buffer_offsets,
                                             i)
    i, @array_tag_resolver =
      create_variable_size_tag_resolver(Flags::ARRAY_VALUE16,
                                        Flags::ARRAY_VALUE32,
                                        buffer_offsets,
                                        i)
    i, @array_position_resolver =
      create_variable_size_position_resolver(Flags::ARRAY_POSITION8,
                                             Flags::ARRAY_POSITION16,
                                             Flags::ARRAY_POSITION32,
                                             buffer_offsets,
                                             i)
    if flagged?(Flags::STRING_VALUE)
      @string_values_offset = buffer_offsets[i]
      i += 1
    end
    i, @string_position_resolver =
      create_variable_size_position_resolver(Flags::STRING_POSITION8,
                                             Flags::STRING_POSITION16,
                                             Flags::STRING_POSITION32,
                                             buffer_offsets,
                                             i)
    if flagged?(Flags::INT16)
      @int16_values_offset = buffer_offsets[i]
      i += 1
    end
    if flagged?(Flags::INT32)
      @int32_values_offset = buffer_offsets[i]
      i += 1
    end
    if flagged?(Flags::INT64)
      @int64_values_offset = buffer_offsets[i]
      i += 1
    end
    if flagged?(Flags::DOUBLE)
      @double_values_offset = buffer_offsets[i]
      i += 1
    end
  end

  def flagged?(flag)
    (@flags & flag) == flag
  end

  def create_variable_size_tag_resolver(flag16,
                                        flag32,
                                        buffer_offsets,
                                        i)
    if flagged?(flag16)
      offset16 = buffer_offsets[i]
      n16 = (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT16_SIZE
      i += 1
    else
      offset16 = 0
      n16 = 0
    end
    if flagged?(flag32)
      offset32 = buffer_offsets[i]
      n32 = (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT32_SIZE
      i += 1
    else
      offset32 = 0
      n32 = 0
    end
    resolver = VariableSizeTagResolver.new(@input,
                                           offset16, n16,
                                           offset32, n32)
    [i, resolver]
  end

  def create_variable_size_position_resolver(flag8,
                                             flag16,
                                             flag32,
                                             buffer_offsets,
                                             i)
    if flagged?(flag8)
      offset8 = buffer_offsets[i]
      n8 = (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT8_SIZE
      i += 1
    else
      offset8 = 0
      n8 = 0
    end
    if flagged?(flag16)
      offset16 = buffer_offsets[i]
      n16 = (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT16_SIZE
      i += 1
    else
      offset16 = 0
      n16 = 0
    end
    if flagged?(flag32)
      offset32 = buffer_offsets[i]
      n32 = (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT32_SIZE
      i += 1
    else
      offset32 = 0
      n_32 = 0
    end
    resolver = VariableSizePositionResolver.new(@input,
                                                offset8, n8,
                                                offset16, n16,
                                                offset32, n32)
    [i, resolver]
  end

  def read_object(index, is_root)
    start, next_start = @object_position_resolver.resolve(index)
    object = {}
    start.step(next_start - 1) do |i|
      tag = @object_tag_resolver.resolve(i * 2)
      name = read_value(tag, false)
      tag = @object_tag_resolver.resolve(i * 2 + 1)
      value = read_value(tag, false)
      object[name] = value
    end
    object
  end

  def read_array(index, is_root)
    start, next_start = @array_position_resolver.resolve(index)
    start.step(next_start - 1).collect do |i|
      tag = @array_tag_resolver.resolve(i)
      read_value(tag, false)
    end
  end

  def read_string(is_embedded, length, data, is_root)
    if is_embedded
      bytes = []
      length.times do |i|
        bytes << ((data >> (8 * i)) & 0b11111111)
      end
      string = bytes.pack("C*")
    else
      if is_root
        string = @input[ROOT_TAG_SIZE..-1]
      else
        index = data
        start, next_start = @string_position_resolver.resolve(index)
        string = @input[@string_values_offset + start, next_start - start]
      end
    end
    string.force_encoding("UTF-8")
    string
  end

  def read_integer(is_embedded, n_bytes, data, is_root)
    if is_embedded
      case n_bytes
      when 1
        [(data & 0xff)].pack("C").unpack1("c")
      when 2
        [(data & 0xffff)].pack("S").unpack1("s")
      when 3
        # 3 bytes is always positive
        data & 0xffffff
      end
    else
      if is_root
        if n_bytes == 1
          @input.unpack1("c", offset: ROOT_TAG_SIZE)
        elsif n_bytes == 2
          @input.unpack1("s", offset: ROOT_TAG_SIZE)
        elsif n_bytes == 3
          @input.unpack1("l", offset: ROOT_TAG_SIZE)
        else
          @input.unpack1("q", offset: ROOT_TAG_SIZE)
        end
      else
        values_offset = data
        if n_bytes == 2
          @input.unpack1("s", offset: @int16_values_offset + data)
        elsif n_bytes == 3
          @input.unpack1("l", offset: @int32_values_offset + data)
        else
          @input.unpack1("q", offset: @int64_values_offset + data)
        end
      end
    end
  end

  def read_double(is_embedded, metadata, data, is_root)
    if is_embedded
      case metadata
      when Metadata::Double::NAN
        Float::NAN
      when Metadata::Double::POSITIVE_INFINITY
        Float::INFINITY
      when Metadata::Double::NEGATIVE_INFINITY
        -Float::INFINITY
      when Metadata::Double::POSITIVE_ZERO
        0.0
      when Metadata::Double::NEGATIVE_ZERO
        -0.0
      end
    else
      if is_root
        @input.unpack1("d", offset: ROOT_TAG_SIZE)
      else
        @input.unpack1("d", offset: @double_values_offset + data)
      end
    end
  end
end

class TestParsedJSON < Test::Unit::TestCase
  def assert_roundtrip(input)
    output = +"".b
    writer = ParsedJSONWriter.new(output, input)
    writer.write
    reader = ParsedJSONReader.new(output)
    if input.is_a?(Float)
      if input.nan?
        assert do
          reader.read.nan?
        end
      elsif input.infinite?
        value = reader.read
        if input.positive?
          assert do
            value.infinite? and value.positive?
          end
        else
          assert do
            value.infinite? and not value.positive?
          end
        end
      else
        assert_equal(input, reader.read)
      end
    else
      assert_equal(input, reader.read)
    end
  end

  def test_constant_true
    assert_roundtrip(true)
  end

  def test_constant_false
    assert_roundtrip(false)
  end

  def test_constant_nil
    assert_roundtrip(nil)
  end

  def test_integer_int8_minimum
    assert_roundtrip(-(2 ** 7))
  end

  def test_integer_int8_maximum
    assert_roundtrip(2 ** 7 - 1)
  end

  def test_integer_int16_minimum
    assert_roundtrip(-(2 ** 15))
  end

  def test_integer_int16_maximum
    assert_roundtrip(2 ** 15 - 1)
  end

  def test_integer_int24_minimum
    assert_roundtrip(-(2 ** 23))
  end

  def test_integer_int24_maximum
    assert_roundtrip(2 ** 23 - 1)
  end

  def test_integer_int32_minimum
    assert_roundtrip(-(2 ** 31))
  end

  def test_integer_int32_maximum
    assert_roundtrip(2 ** 31 - 1)
  end

  def test_integer_int64_minimum
    assert_roundtrip(-(2 ** 63))
  end

  def test_integer_int64_maximum
    assert_roundtrip(2 ** 63 - 1)
  end

  def test_double_negative
    assert_roundtrip(-2.9)
  end

  def test_double_positive
    assert_roundtrip(2.9)
  end

  def test_double_nan
    assert_roundtrip(Float::NAN)
  end

  def test_double_infinity_positive
    assert_roundtrip(Float::INFINITY)
  end

  def test_double_infinity_negative
    assert_roundtrip(-Float::INFINITY)
  end

  def test_double_zero_positive
    assert_roundtrip(0.0)
  end

  def test_double_zero_negative
    assert_roundtrip(-0.0)
  end

  def test_string
    assert_roundtrip("hello")
  end

  def test_string_embed_tag16
    assert_roundtrip("a")
  end

  def test_string_embed_tag32
    elements = ["abc"] * 258
    assert_roundtrip(elements)
  end

  def test_string_embed_empty
    assert_roundtrip("")
  end

  def test_string_size8_size16
    assert_roundtrip(["a" * 255, "abcde"])
  end

  def test_string_size8_size16_size32
    assert_roundtrip(["a" * 255, "a" * (65535 - 255), "abcde"])
  end

  def test_string_size8_size32
    assert_roundtrip(["a" * 255, "a" * 65535, "abcde"])
  end

  def test_string_size16_size32
    assert_roundtrip(["a" * 65535, "abcde"])
  end

  def test_string_size16
    assert_roundtrip(["a" * 256, "abcde"])
  end

  def test_string_size32
    assert_roundtrip(["a" * 65536, "abcde"])
  end

  def test_array
    assert_roundtrip([true, false, nil, -1, 1, 2.9])
  end

  def test_array_nested
    assert_roundtrip([
                       true,
                       [
                         false,
                         nil,
                         [-1, 1, 2.9],
                         ["string"],
                         nil,
                       ],
                       {
                         "string" => "world",
                         "sub_array" => [
                           nil,
                           {
                             "sub_object" => false,
                           },
                           ["sub_string", -2.9],
                         ],
                         "integer" => -29,
                       },
                       false,
                     ])
  end

  def test_array_size8_size16
    assert_roundtrip([(0..257).to_a])
  end

  def test_array_size8_size16_size32
    assert_roundtrip([(0..257).to_a, (0..65535).to_a])
  end

  def test_array_size8_size32
    assert_roundtrip([(0..65535).to_a])
  end

  def test_array_size16_size32
    assert_roundtrip((0..257).to_a + [(0..65535).to_a])
  end

  def test_array_size16
    assert_roundtrip((0..257).to_a)
  end

  def test_array_size64
    assert_roundtrip((0..65535).to_a)
  end

  def test_object
    assert_roundtrip({
                       "string" => "world",
                       "integer" => 29,
                       "true" => true,
                     })
  end

  def test_object_nested
    assert_roundtrip({
                       "string" => "world",
                       "integer" => 29,
                       "object" => {
                         "null" => nil,
                         "array" => [
                           nil,
                           {
                             "array_object_array" => [-2.9],
                           },
                           true,
                         ],
                         "sub_object" => {
                           "false" => false,
                           "double" => 2.9,
                         },
                         "integer" => 2929,
                       },
                       "true" => true,
                     })
  end

  def test_object_long_tag32
    object = {}
    76.times do |i|
      object[i.to_s] = i
    end
    assert_roundtrip(object)
  end

  def test_object_size8_size16
    objects = []
    256.times do |i|
      objects << {i.to_s => i}
    end
    assert_roundtrip(objects)
  end

  def test_object_size8_size16_size32
    objects = []
    65536.times do |i|
      objects << {i.to_s => i}
    end
    assert_roundtrip(objects)
  end


  def test_object_size8_size32
    objects = []
    255.times do |i|
      objects << {i.to_s => i}
    end
    object = {}
    65536.times do |i|
      object[i.to_s] = i
    end
    objects << object
    255.step(65535) do |i|
      objects << {i.to_s => i}
    end
    assert_roundtrip(objects)
  end

  def test_object_size16_size32
    object = {}
    256.times do |i|
      object[i.to_s] = i
    end
    objects = [object]
    65535.times do |i|
      objects << {i.to_s => i}
    end
    assert_roundtrip(objects)
  end
  def test_object_size16
    object = {}
    256.times do |i|
      object[i.to_s] = i
    end
    assert_roundtrip(object)
  end

  def test_object_size32
    object = {}
    65536.times do |i|
      object[i.to_s] = i
    end
    assert_roundtrip(object)
  end
end
