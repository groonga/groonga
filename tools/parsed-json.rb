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
  # Format:
  #
  # +-----+---------+--------+
  # | TAG | BUFFERS | FOOTER |
  # +-----+---------+--------+
  #
  # TAG: The tag (described later) of the target JSON.
  #
  # BUFFERS: It contains the following buffers. They can be accessed
  #          by offsets in the FOOTER:
  #   * OBJECT values buffer
  #   * OBJECT offsets buffer
  #   * ARRAY values buffer
  #   * ARRAY offsets buffer
  #   * STRING values buffer
  #   * STRING offsets buffer
  #   * INTEGER(int32) values buffer
  #   * INTEGER(int64) values buffer
  #   * DOUBLE values buffer
  #
  # FOOTER: 36 bytes (sizeof(uint32) * 9 buffers).
  #   It contains the following offsets:
  #     * OBJECT values buffer offset: uint32
  #     * OBJECT offsets buffer offset: uint32
  #     * ARRAY values buffer offset: uint32
  #     * ARRAY offsets buffer offset: uint32
  #     * STRING values buffer offset: uint32
  #     * STRING offsets buffer offset: uint32
  #     * INTEGER(int32) values buffer offset: uint32
  #     * INTEGER(int64) values buffer offset: uint32
  #     * DOUBLE values buffer offset: uint32

  FOOTER_SIZE = 4 * 9 # sizeof(uint32) * 9 buffers

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

  # tag: 32 bits: DDDDDDDD DDDDDDD DDDDDDDD MMMMETTT
  #
  # TTT: Type
  # E: Embedded or not (1: embedded, 0: not embedded)
  # MMMM: Metadata
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
  #     If E is 1:
  #       (-2^15) - (2^27-1): Itself. We can't embed int32 negative numbers.
  #     Else:
  #       (-2^31) - (2^31-1): Offset in int32 values.
  #       (-2^63) - (2^63-1): Offset in int64 values.
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
  #   * INTEGER(int32)
  #   * INTEGER(int64)
  #   * DOUBLE
  #
  # OBJECT:
  #   Written in breadth-first order for nested objects.
  #
  #   OBJECT uses 2 buffers:
  #     * Values buffer:
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
  #   ARRAY uses 2 buffers:
  #     * Values buffer:
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

  def initialize(output, target)
    @output = output
    @target = target
    @object_values = +""
    @object_offsets = [0].pack("L")
    @array_values = +""
    @array_offsets = [0].pack("L")
    @string_values = +""
    @string_offsets = [0].pack("L")
    @int32_values = +""
    @int64_values = +""
    @double_values = +""
  end

  def write
    write_value(@output, @target)
    footer = +"".b
    offset = @output.bytesize
    [
      @object_values,
      @object_offsets,
      @array_values,
      @array_offsets,
      @string_values,
      @string_offsets,
      @int32_values,
      @int64_values,
      @double_values,
    ].each do |values|
      @output << values
      footer << [offset].pack("L")
      offset += values.bytesize
    end
    @output << footer
  end

  private
  def append_container(output, target)
    targets = [target]
    outputs = [output]
    until targets.empty?
      target = targets.shift
      output = outputs.shift
      case target
      when Hash
        offset = @object_offsets.bytesize - 4 # sizeof(uint32_t)
        last_object_offset = @object_offsets.unpack1("L", offset: offset)
        @object_offsets << [last_object_offset + target.size].pack("L")
        write_tag(output, pack_tag(Type::OBJECT, false, 0, offset))
        target.each do |name, value|
          targets << name
          outputs << @object_values
          targets << value
          outputs << @object_values
        end
      when Array
        offset = @array_offsets.bytesize - 4 # sizeof(uint32_t)
        last_array_offset = @array_offsets.unpack1("L", offset: offset)
        @array_offsets << [last_array_offset + target.size].pack("L")
        write_tag(output, pack_tag(Type::ARRAY, false, 0, offset))
        target.each do |value|
          targets << value
          outputs << @array_values
        end
      else
        write_value(output, target)
      end
    end
  end

  def append_string(string)
    offset = @string_offsets.bytesize - 4 # sizeof(uint32_t)
    @string_values << string
    last_string_offset = @string_offsets.unpack1("L", offset: offset)
    @string_offsets << [last_string_offset + string.bytesize].pack("L")
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

  def write_tag(output, tag)
    output << [tag].pack("L")
  end

  def write_value(output, value)
    case value
    when Hash # object
      append_container(output, value)
    when Array
      append_container(output, value)
    when String
      if value.bytesize <= 3
        data = 0
        value.unpack("C*").each_with_index do |character, i|
          data |= (character << (8 * (2 - i)))
        end
        write_tag(output, pack_tag(Type::STRING, true, value.bytesize, data))
      else
        offset = append_string(value)
        write_tag(output, pack_tag(Type::STRING, false, 0, offset))
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
        write_tag(output, pack_tag(Type::DOUBLE, false, 0, offset))
        return
      end

      if -(2 ** 15) <= value and value <= (2 ** 23 - 1)
        write_tag(output, pack_tag(Type::INTEGER, true, n_bytes, value))
      else
        if n_bytes == 3
          offset = append_int32(value)
        else
          offset = append_int64(value)
        end
        write_tag(output, pack_tag(Type::INTEGER, false, n_bytes, offset))
      end
    when Float
      offset = append_double(value)
      write_tag(output, pack_tag(Type::DOUBLE, false, 0, offset))
    when true
      write_tag(output,
                pack_tag(Type::CONSTANT, true, Metadata::Constant::TRUE, 0))
    when false
      write_tag(output,
                pack_tag(Type::CONSTANT, true, Metadata::Constant::FALSE, 0))
    when nil
      write_tag(output,
                pack_tag(Type::CONSTANT, true, Metadata::Constant::NULL, 0))
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
    footer_offset = @input.bytesize - FOOTER_SIZE

    values_offset_offset = footer_offset
    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @object_values_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @object_offsets_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @array_values_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @array_offsets_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @string_values_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @string_offsets_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @int32_values_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @int64_values_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    values_offset = @input.unpack1("L", offset: values_offset_offset)
    @double_values_offset = values_offset
    values_offset_offset += 4 # sizeof(uint32_t)

    read_value(0)
  end

  private
  def read_object(values_offset)
    offsets_offset = @object_offsets_offset + values_offset
    start, next_start = @input.unpack("L2", offset: offsets_offset)
    n_members = next_start - start
    object = {}
    n_members.times do |i|
      # 8 = sizeof(uint32_t) + sizeof(uint32_t)
      base_offset = @object_values_offset + ((start + i) * 8)
      name = read_value(base_offset)
      value = read_value(base_offset + 4) # sizeof(uint32_t)
      object[name] = value
    end
    object
  end

  def read_array(values_offset)
    offsets_offset = @array_offsets_offset + values_offset
    start, next_start = @input.unpack("L2", offset: offsets_offset)
    n_elements = next_start - start
    n_elements.times.collect do |i|
      read_value(@array_values_offset + ((start + i) * 4)) # sizeof(uint32_t)
    end
  end

  def read_string(is_embedded, length, data)
    if is_embedded
      bytes = []
      length.times do |i|
        bytes << ((data >> (16 - (8 * i))) & 0b11111111)
      end
      bytes.pack("C*")
    else
      values_offset = data
      offsets_offset = @string_offsets_offset + values_offset
      start, next_start = @input.unpack("L2", offset: offsets_offset)
      @input[@string_values_offset + start, next_start - start]
    end
  end

  def read_integer(is_embedded, byte_width, data)
    if is_embedded
      case byte_width
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
      if byte_width == 3
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
    tag = @input.unpack1("L", offset: offset)
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
end
