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
  # +-----+---------+----------------+--------------+
  # | TAG | BUFFERS | BUFFER_OFFSETS | USED_BUFFERS |
  # +-----+---------+----------------+--------------+
  #
  # TAG: The tag (described later) of the target JSON.
  #
  # BUFFERS: It contains the following buffers. They can be accessed
  #          by offsets in the BUFFER_OFFSETS
  #   * TAG 32 bits buffer
  #   * OBJECT 16 bits values buffer
  #   * OBJECT 32 bits values buffer
  #   * OBJECT offsets buffer
  #   * ARRAY 16 bits values buffer
  #   * ARRAY 32 bits values buffer
  #   * ARRAY offsets buffer
  #   * STRING values buffer
  #   * STRING offsets buffer
  #   * INTEGER(int16) values buffer
  #   * INTEGER(int32) values buffer
  #   * INTEGER(int64) values buffer
  #   * DOUBLE values buffer
  #
  # USED_BUFFERS: 2 bytes flags
  #   * 0b00000000_00000001: TAG 32 bits buffer is used
  #   * 0b00000000_00000010: OBJECT 16 bits values buffer is used
  #   * 0b00000000_00000100: OBJECT 32 bits values buffer is used
  #   * 0b00000000_00001000: OBJECT offsets buffer is used
  #   * 0b00000000_00010000: ARRAY 16 bits values buffer is used
  #   * 0b00000000_00100000: ARRAY 32 bits values buffer is used
  #   * 0b00000000_01000000: ARRAY offsets buffer is used
  #   * 0b00000000_10000000: STRING values/offsets buffers are used
  #   * 0b00000001_00000000: INT16 values buffer is used
  #   * 0b00000010_00000000: INT32 values buffer is used
  #   * 0b00000100_00000000: INT64 values buffer is used
  #   * 0b00001000_00000000: DOUBLE values buffer is used
  #
  # BUFFER_OFFSETS:
  #   It contains the following offsets in this order:
  #     * If USED_BUFFERS has 0b00000000_00000001:
  #       * TAG 32 bits buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000000_00000010:
  #       * OBJECT 16 bits values buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000000_00000100:
  #       * OBJECT 32 bits values buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000000_00001000:
  #       * OBJECT offsets buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000000_00010000:
  #       * ARRAY 16 bits values buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000000_00100000:
  #       * ARRAY 32 bits values buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000000_01000000:
  #       * ARRAY offsets buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000000_10000000:
  #       * STRING values buffer offset: uint32
  #       * STRING offsets buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000001_00000000:
  #       * INTEGER(int16) values buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000010_00000000:
  #       * INTEGER(int32) values buffer offset: uint32
  #     * If USED_BUFFERS has 0b00000100_00000000:
  #       * INTEGER(int64) values buffer offset: uint32
  #     * If USED_BUFFERS has 0b00001000_00000000:
  #       * DOUBLE values buffer offset: uint32

  module UsedBuffer
    SIZE = UINT16_SIZE

    TAG32           = 0b00000000_00000001
    OBJECT_VALUE16  = 0b00000000_00000010
    OBJECT_VALUE32  = 0b00000000_00000100
    OBJECT_OFFSET8  = 0b00000000_00001000
    OBJECT_OFFSET16 = 0b00000000_00010000
    OBJECT_OFFSET32 = 0b00000000_00100000
    ARRAY_VALUE16   = 0b00000000_01000000
    ARRAY_VALUE32   = 0b00000000_10000000
    ARRAY_OFFSET    = 0b00000001_00000000
    STRING          = 0b00000010_00000000
    INT16           = 0b00000100_00000000
    INT32           = 0b00001000_00000000
    INT64           = 0b00010000_00000000
    DOUBLE          = 0b00100000_00000000
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
    module Constant
      TRUE = 0
      FALSE = 1
      NULL = 2

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
  #     Offset in object values.
  #     E is always 0.
  #     MMMM is always 0000.
  #   ARRAY:
  #     Offset in array values.
  #     E is always 0.
  #     MMMM is always 0000.
  #   STRING:
  #     If E is 1:
  #       0-3 bytes string itself.
  #       MMMM shows the number of bytes.
  #     Else:
  #       Offset in string offsets.
  #       MMMM is 0000.
  #   INTEGER:
  #     MMMM shows the byte width.
  #     If TAG is 16 bits:
  #       If E is 1:
  #         -(2^7) - (2^7-1): Itself. We can embed only int32 values.
  #       Else:
  #         -(2^15) - (2^15-1): Offset in int16 values.
  #         -(2^31) - (2^31-1): Offset in int32 values.
  #         -(2^63) - (2^63-1): Offset in int64 values.
  #     Else:
  #       If E is 1:
  #         -(2^15) - (2^27-1): Itself. We can't embed int32 negative numbers.
  #       Else:
  #         -(2^31) - (2^31-1): Offset in int32 values.
  #         -(2^63) - (2^63-1): Offset in int64 values.
  #   DOUBLE:  Offset in double values. E is always 0.
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
  #     * Offsets buffer:
  #       * uint32_t
  #       * [
  #           0,
  #           0 + N_MEMBERS_OF_1ST_OBJECT,
  #           0 + N_MEMBERS_OF_1ST_OBJECT + N_MEMBERS_OF_2ND_OBJECT,
  #           ...,
  #         ]
  #       * offsets[N - 1] - offsets[N]:
  #         The number of members of the Nth object.
  #       * values[offsets[N]..(offsets[N - 1] - offsets[N])]:
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
  #     * Offsets buffer:
  #       * uint32_t
  #       * [
  #           0,
  #           0 + N_MEMBERS_OF_1ST_ARRAY,
  #           0 + N_MEMBERS_OF_1ST_ARRAY + N_MEMBERS_OF_2ND_ARRAY,
  #           ...,
  #         ]
  #       * offsets[N - 1] - offsets[N]:
  #         The number of elements of the Nth array.
  #       * values[offsets[N]..(offsets[N - 1] - offsets[N])]:
  #         The elements of the Nth array.
  #
  # STRING
  #   STRING uses 2 buffers:
  #     * Values buffer: String itself.
  #     * Offsets buffer:
  #       * uint32_t
  #       * [
  #           0,
  #           0 + BYTE_SIZE_OF_1ST_STRING,
  #           0 + BYTE_SIZE_OF_1ST_STRING + BYTE_SIZE_OF_2ND_STRING,
  #           ...,
  #         ]
  #       * offsets[N - 1] - offsets[N]:
  #         The byte size of the Nth string.
  #       * values[offsets[N]..(offsets[N - 1] - offsets[N])]:
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

  class OffsetWriter
    include ParsedJSON

    attr_reader :buffer
    attr_reader :size8
    attr_reader :size16
    attr_reader :size32
    def initialize(buffer)
      @buffer = buffer
      @i = 0
      @last_offset = 0
      @size8 = 0
      @size16 = 0
      @size32 = 0
    end

    def write(offset_diff)
      i = @i
      @i += 1
      last_offset = @last_offset
      offset = @last_offset + offset_diff
      if @size32 > 0 or offset > 65535
        @buffer << [offset].pack("L")
        @size32 += UINT32_SIZE
      elsif @size16 > 0 or offset > 255
        @buffer << [offset].pack("S")
        @size16 += UINT16_SIZE
      else
        @buffer << [offset].pack("C")
        @size8 += UINT8_SIZE
      end
      @last_offset = offset
      i
    end
  end

  def initialize(output, target)
    @output = output
    @target = target
    @tag_writer = TagWriter.new(:tag, @output)
    @object_values = +"".b
    @object_values_writer = TagWriter.new(:object, @object_values)
    @object_offsets = +"".b
    @object_offsets_writer = OffsetWriter.new(@object_offsets)
    @array_values = +"".b
    @array_values_writer = TagWriter.new(:array, @array_values)
    @array_offsets = [0].pack("L").b
    @string_values = +"".b
    @string_offsets = [0].pack("L").b
    @int16_values = +"".b
    @int32_values = +"".b
    @int64_values = +"".b
    @double_values = +"".b
  end

  def write
    write_value(@tag_writer, @target)
    used_buffers = 0
    buffer_offsets = +"".b
    offset = @tag_writer.size16
    if @tag_writer.size32 > 0
      used_buffers |= UsedBuffer::TAG32
      buffer_offsets << [@tag_writer.size32].pack("L")
      offset += @tag_writer.size32
    end
    if @object_values_writer.size16 > 0
      used_buffers |= UsedBuffer::OBJECT_VALUE16
      @output << @object_values
      buffer_offsets << [offset].pack("L")
      offset += @object_values_writer.size16
    end
    if @object_values_writer.size32 > 0
      used_buffers |= UsedBuffer::OBJECT_VALUE32
      buffer_offsets << [offset].pack("L")
      offset += @object_values_writer.size32
    end
    unless @object_offsets.empty?
      @output << @object_offsets
    end
    if @object_offsets_writer.size8 > 0
      used_buffers |= UsedBuffer::OBJECT_OFFSET8
      buffer_offsets << [offset].pack("L")
      offset += @object_offsets_writer.size8
    end
    if @object_offsets_writer.size16 > 0
      used_buffers |= UsedBuffer::OBJECT_OFFSET16
      buffer_offsets << [offset].pack("L")
      offset += @object_offsets_writer.size16
    end
    if @object_offsets_writer.size32 > 0
      used_buffers |= UsedBuffer::OBJECT_OFFSET32
      buffer_offsets << [offset].pack("L")
      offset += @object_offsets_writer.size32
    end
    if @array_values_writer.size16 > 0
      used_buffers |= UsedBuffer::ARRAY_VALUE16
      @output << @array_values
      buffer_offsets << [offset].pack("L")
      offset += @array_values_writer.size16
    end
    if @array_values_writer.size32 > 0
      used_buffers |= UsedBuffer::ARRAY_VALUE32
      buffer_offsets << [offset].pack("L")
      offset += @array_values_writer.size32
    end
    unless @array_values.empty?
      used_buffers |= UsedBuffer::ARRAY_OFFSET
      @output << @array_offsets
      buffer_offsets << [offset].pack("L")
      offset += @array_offsets.bytesize
    end
    unless @string_values.empty?
      used_buffers |= UsedBuffer::STRING
      @output << @string_values
      buffer_offsets << [offset].pack("L")
      offset += @string_values.bytesize
      @output << @string_offsets
      buffer_offsets << [offset].pack("L")
      offset += @string_offsets.bytesize
    end
    unless @int16_values.empty?
      used_buffers |= UsedBuffer::INT16
      @output << @int16_values
      buffer_offsets << [offset].pack("L")
      offset += @int16_values.bytesize
    end
    unless @int32_values.empty?
      used_buffers |= UsedBuffer::INT32
      @output << @int32_values
      buffer_offsets << [offset].pack("L")
      offset += @int32_values.bytesize
    end
    unless @int64_values.empty?
      used_buffers |= UsedBuffer::INT64
      @output << @int64_values
      buffer_offsets << [offset].pack("L")
      offset += @int64_values.bytesize
    end
    unless @double_values.empty?
      used_buffers |= UsedBuffer::DOUBLE
      @output << @double_values
      buffer_offsets << [offset].pack("L")
      offset += @double_values.bytesize
    end
    @output << buffer_offsets
    @output << [used_buffers].pack("S")
  end

  private
  def write_container(tag_writer, target)
    targets = [target]
    tag_writers = [tag_writer]
    until targets.empty?
      target = targets.shift
      tag_writer = tag_writers.shift
      case target
      when Hash
        offset = @object_offsets_writer.write(target.size)
        tag_writer.write(Type::OBJECT, false, 0, offset)
        target.each do |name, value|
          targets << name
          tag_writers << @object_values_writer
          targets << value
          tag_writers << @object_values_writer
        end
      when Array
        offset = @array_offsets.bytesize - UINT32_SIZE
        last_array_offset = @array_offsets.unpack1("L", offset: offset)
        @array_offsets << [last_array_offset + target.size].pack("L")
        tag_writer.write(Type::ARRAY, false, 0, offset)
        target.each do |value|
          targets << value
          tag_writers << @array_values_writer
        end
      else
        write_value(tag_writer, target)
      end
    end
  end

  def append_string(string)
    offset = @string_offsets.bytesize - UINT32_SIZE
    @string_values.append_as_bytes(string)
    last_string_offset = @string_offsets.unpack1("L", offset: offset)
    @string_offsets << [last_string_offset + string.bytesize].pack("L")
    offset
  end

  def append_int16(int16)
    offset = @int16_values.bytesize
    @int16_values << [int16].pack("s")
    offset
  end

  def append_int32(int32)
    offset = @int32_values.bytesize
    @int32_values << [int32].pack("l")
    offset
  end

  def append_int64(int64)
    offset = @int64_values.bytesize
    @int64_values << [int64].pack("q")
    offset
  end

  def append_double(double)
    offset = @double_values.bytesize
    @double_values << [double].pack("d")
    offset
  end

  def write_value(tag_writer, value)
    case value
    when Hash # object
      write_container(tag_writer, value)
    when Array
      write_container(tag_writer, value)
    when String
      if tag_writer.size32 > 3
        max_embeddable_size = 3
      else
        max_embeddable_size = 1
      end
      if value.bytesize <= max_embeddable_size
        data = 0
        value.unpack("C*").each.with_index do |character, i|
          data |= character << (8 * i)
        end
        tag_writer.write(Type::STRING, true, value.bytesize, data)
      else
        offset = append_string(value)
        tag_writer.write(Type::STRING, false, 0, offset)
      end
    when Integer
      if -(2 ** 7) <= value and value <= (2 ** 7 - 1)
        n_bytes = 1
      elsif -(2 ** 15) <= value and value <= (2 ** 15 - 1)
        n_bytes = 2
      elsif -(2 ** 31) <= value and value <= (2 ** 31 - 1)
        n_bytes = 3
      elsif -(2 ** 63) <= value and value <= (2 ** 63 - 1)
        n_bytes = 4
      else
        offset = append_double(value.to_f)
        tag_writer.write(Type::DOUBLE, false, 0, offset)
        return
      end

      is_tag32 = (tag_writer.size32 > 0)
      if is_tag32
        is_embeddable = (-(2 ** 15) <= value and value <= (2 ** 23 - 1))
      else
        is_embeddable = (n_bytes == 1)
      end
      if is_embeddable
        if not is_tag32 and value < 0
          value += 256
        end
        tag_writer.write(Type::INTEGER, true, n_bytes, value)
      else
        if n_bytes == 2
          offset = append_int16(value)
        elsif n_bytes == 3
          offset = append_int32(value)
        else
          offset = append_int64(value)
        end
        tag_writer.write(Type::INTEGER, false, n_bytes, offset)
      end
    when Float
      offset = append_double(value)
      tag_writer.write(Type::DOUBLE, false, 0, offset)
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
end

class ParsedJSONReader
  include ParsedJSON

  def initialize(input)
    @input = input
  end

  def read
    used_buffers_offset = @input.bytesize - UsedBuffer::SIZE
    @used_buffers = @input.unpack1("S", offset: used_buffers_offset)

    n_buffer_offsets = 0
    if buffer_used?(UsedBuffer::TAG32)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::OBJECT_VALUE16)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::OBJECT_VALUE32)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::OBJECT_OFFSET8)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::OBJECT_OFFSET16)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::OBJECT_OFFSET32)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::ARRAY_VALUE16)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::ARRAY_VALUE32)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::ARRAY_OFFSET)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::STRING)
      n_buffer_offsets += 2
    end
    if buffer_used?(UsedBuffer::INT16)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::INT32)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::INT64)
      n_buffer_offsets += 1
    end
    if buffer_used?(UsedBuffer::DOUBLE)
      n_buffer_offsets += 1
    end

    buffer_offsets_offset =
      used_buffers_offset - (UINT32_SIZE * n_buffer_offsets)
    buffer_offsets = @input.unpack("L#{n_buffer_offsets}",
                                   offset: buffer_offsets_offset)
    @tag16_ranges = []
    buffer_offsets << used_buffers_offset
    i = 0
    @tag16_ranges << (0...buffer_offsets[i])
    if buffer_used?(UsedBuffer::TAG32)
      @tag32_offset = buffer_offsets[i]
      i += 1
    else
      @tag32_offset = 0
    end
    if buffer_used?(UsedBuffer::OBJECT_VALUE16)
      @object16_values_offset = buffer_offsets[i]
      @tag16_ranges << (buffer_offsets[i]...buffer_offsets[i + 1])
      @n_object16_values =
        (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT16_SIZE
      i += 1
    else
      @object16_values_offset = 0
      @n_object16_values = 0
    end
    if buffer_used?(UsedBuffer::OBJECT_VALUE32)
      @object32_values_offset = buffer_offsets[i]
      i += 1
    else
      @object32_values_offset = 0
    end
    if buffer_used?(UsedBuffer::OBJECT_OFFSET8)
      @object8_offsets_offset = buffer_offsets[i]
      @n_object8_offsets =
        (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT8_SIZE
      i += 1
    else
      @object8_offsets_offset = 0
      @n_object8_offsets = 0
    end
    if buffer_used?(UsedBuffer::OBJECT_OFFSET16)
      @object16_offsets_offset = buffer_offsets[i]
      @n_object16_offsets =
        (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT16_SIZE
      i += 1
    else
      @object16_offsets_offset = 0
      @n_object16_offsets = 0
    end
    if buffer_used?(UsedBuffer::OBJECT_OFFSET32)
      @object32_offsets_offset = buffer_offsets[i]
      @n_object32_offsets =
        (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT32_SIZE
      i += 1
    else
      @object32_offsets_offset = 0
      @n_object32_offsets = 0
    end
    if buffer_used?(UsedBuffer::ARRAY_VALUE16)
      @array16_values_offset = buffer_offsets[i]
      @tag16_ranges << (buffer_offsets[i]...buffer_offsets[i + 1])
      @n_array16_values =
        (buffer_offsets[i + 1] - buffer_offsets[i]) / UINT16_SIZE
      i += 1
    else
      @array16_values_offset = 0
      @n_array16_values = 0
    end
    if buffer_used?(UsedBuffer::ARRAY_VALUE32)
      @array32_values_offset = buffer_offsets[i]
      i += 1
    else
      @array32_values_offset = 0
    end
    if buffer_used?(UsedBuffer::ARRAY_OFFSET)
      @array_offsets_offset = buffer_offsets[i]
      i += 1
    end
    if buffer_used?(UsedBuffer::STRING)
      @string_values_offset = buffer_offsets[i]
      i += 1
      @string_offsets_offset = buffer_offsets[i]
      i += 1
    end
    if buffer_used?(UsedBuffer::INT16)
      @int16_values_offset = buffer_offsets[i]
      i += 1
    end
    if buffer_used?(UsedBuffer::INT32)
      @int32_values_offset = buffer_offsets[i]
      i += 1
    end
    if buffer_used?(UsedBuffer::INT64)
      @int64_values_offset = buffer_offsets[i]
      i += 1
    end
    if buffer_used?(UsedBuffer::DOUBLE)
      @double_values_offset = buffer_offsets[i]
      i += 1
    end

    read_value(0)
  end

  private
  def buffer_used?(flag)
    (@used_buffers & flag) == flag
  end

  def read_object(i_value)
    if i_value < @n_object8_offsets
      offsets_offset = @object8_offsets_offset + i_value
      if i_value == 0
        start = 0
        next_start = @input.unpack1("C", offset: offsets_offset)
      else
        start, next_start = @input.unpack("C2",
                                          offset: offsets_offset - UINT8_SIZE)
      end
    elsif i_value < @n_object8_offsets + @n_object16_offsets
      offsets_offset =
        @object16_offsets_offset +
        ((i_value - @n_object8_offsets) * UINT16_SIZE)
      if i_value == 0
        start = 0
        next_start = @input.unpack1("S", offset: offsets_offset)
      elsif i_value == @n_object8_offsets
        last_offset8_offset =
          @object8_offsets_offset + (@n_object8_offsets - 1) * UINT8_SIZE
        start = @input.unpack1("C", offset: last_offset8_offset)
        next_start = @input.unpack1("S", offset: offsets_offset)
      else
        start, next_start = @input.unpack("S2",
                                          offset: offsets_offset - UINT16_SIZE)
      end
    else
      offsets_offset =
        @object32_offsets_offset +
        ((i_value - @n_object8_offsets - @n_object16_offsets) * UINT32_SIZE)
      if i_value == 0
        start = 0
        next_start = @input.unpack1("L", offset: offsets_offset)
      elsif i_value == @n_object8_offsets + @n_object16_offsets
        if @n_object16_offsets == 0
          last_offset8_offset =
            @object8_offsets_offset + (@n_object8_offsets * UINT8_SIZE)
          start = @input.unpack1("C", offset: last_offset8_offset)
        else
          last_offset16_offset =
            @object16_offsets_offset + ((@n_object16_offsets - 1) * UINT16_SIZE)
          start = @input.unpack1("S", offset: last_offset16_offset)
        end
        next_start = @input.unpack1("L", offset: offsets_offset)
      else
        start, next_start = @input.unpack("L2",
                                          offset: offsets_offset - UINT32_SIZE)
      end
    end
    n_skip_values = start * 2
    n_members = next_start - start
    if n_skip_values < @n_object16_values
      base_values_offset = @object16_values_offset
      base_values_offset += n_skip_values * UINT16_SIZE
    else
      base_values_offset = @object32_values_offset
      base_values_offset += (n_skip_values - @n_object16_values) * UINT32_SIZE
    end
    object = {}
    offset = base_values_offset
    n_members.times do |i|
      name = read_value(offset)
      if n_skip_values + (i * 2) < @n_object16_values
        member_size = UINT16_SIZE
      else
        member_size = UINT32_SIZE
      end
      offset += member_size
      value = read_value(offset)
      if n_skip_values + (i * 2) + 1 < @n_object16_values
        member_size = UINT16_SIZE
      else
        member_size = UINT32_SIZE
      end
      offset += member_size
      object[name] = value
    end
    object
  end

  def read_array(values_offset)
    offsets_offset = @array_offsets_offset + values_offset
    start, next_start = @input.unpack("L2", offset: offsets_offset)
    n_elements = next_start - start
    if start < @n_array16_values
      element_size = UINT16_SIZE
      base_values_offset = @array16_values_offset
      base_values_offset += start * element_size
    else
      element_size = UINT32_SIZE
      base_values_offset = @array32_values_offset
      base_values_offset += (start - @n_array16_values) * element_size
    end
    offset = base_values_offset
    n_elements.times.collect do |i|
      if start + i < @n_array16_values
        element_size = UINT16_SIZE
      else
        element_size = UINT32_SIZE
      end
      value = read_value(offset)
      offset += element_size
      value
    end
  end

  def read_string(is_embedded, length, data)
    if is_embedded
      bytes = []
      length.times do |i|
        bytes << ((data >> (8 * i)) & 0b11111111)
      end
      bytes.pack("C*")
    else
      values_offset = data
      offsets_offset = @string_offsets_offset + values_offset
      start, next_start = @input.unpack("L2", offset: offsets_offset)
      @input[@string_values_offset + start, next_start - start]
    end
  end

  def read_integer(is_embedded, n_bytes, data)
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

  def read_double(values_offset)
    @input.unpack1("d", offset: @double_values_offset + values_offset)
  end

  def read_value(offset)
    if @tag16_ranges.any? {|range| range.cover?(offset)}
      tag_size = 16
      tag = @input.unpack1("S", offset: offset)
    else
      tag_size = 32
      tag = @input.unpack1("L", offset: offset)
    end
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

    case type
    when Type::OBJECT
      read_object(data)
    when Type::ARRAY
      read_array(data)
    when Type::STRING
      read_string(is_embedded, metadata, data)
    when Type::INTEGER
      read_integer(is_embedded, metadata, data)
    when Type::DOUBLE
      read_double(data)
    else
      raise "Unknown type: #{type.inspect}"
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

  def test_string
    assert_roundtrip("hello")
  end

  def test_string_embed
    assert_roundtrip("abc")
  end

  def test_string_embed_empty
    assert_roundtrip("")
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

  def test_array_long
    assert_roundtrip((0..257).to_a)
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

  def test_object_long_offset16
    objects = []
    256.times do |i|
      objects << {i.to_s => i}
    end
    assert_roundtrip(objects)
  end

  def test_object_long_offset16_only
    object = {}
    256.times do |i|
      object[i.to_s] = i
    end
    assert_roundtrip(object)
  end

  def test_object_long_offset32
    objects = []
    65536.times do |i|
      objects << {i.to_s => i}
    end
    assert_roundtrip(objects)
  end

  def test_object_long_offset32_only
    object = {}
    65536.times do |i|
      object[i.to_s] = i
    end
    assert_roundtrip(object)
  end

  def test_object_long_offset8_offset32
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

  def test_object_long_offset16_offset32
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
end
