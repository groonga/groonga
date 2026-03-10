/*
  Copyright (C) 2025-2026  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn_ctx.h"

#ifdef GRN_WITH_SIMDJSON
#  include <cmath>
#  include <limits>
#  include <queue>
#  include <string>

#  include <simdjson.h>
#endif

#include <memory>
#include <stack>
#include <utility>
#include <vector>

// See tools/parsed-json.rb for format details.

namespace {
  enum class Flag {
    kObjectPosition8 = 0b00000000'00000000'00000001,
    kObjectPosition16 = 0b00000000'00000000'00000010,
    kObjectPosition32 = 0b00000000'00000000'00000100,
    kObjectKey = 0b00000000'00000000'00001000,
    kObjectKeyPosition8 = 0b00000000'00000000'00010000,
    kObjectKeyPosition16 = 0b00000000'00000000'00100000,
    kObjectKeyPosition32 = 0b00000000'00000000'01000000,
    kObjectValueTag16 = 0b00000000'00000000'10000000,
    kObjectValueTag32 = 0b00000000'00000001'00000000,
    kArrayPosition8 = 0b00000000'00000010'00000000,
    kArrayPosition16 = 0b00000000'00000100'00000000,
    kArrayPosition32 = 0b00000000'00001000'00000000,
    kArrayTag16 = 0b00000000'00010000'00000000,
    kArrayTag32 = 0b00000000'00100000'00000000,
    kStringValue = 0b00000000'01000000'00000000,
    kStringPosition8 = 0b00000000'10000000'00000000,
    kStringPosition16 = 0b00000001'00000000'00000000,
    kStringPosition32 = 0b00000010'00000000'00000000,
    kInt16 = 0b00000100'00000000'00000000,
    kInt32 = 0b00001000'10000000'00000000,
    kInt64 = 0b00010000'00000000'00000000,
    kDouble = 0b00100000'00000000'00000000,
    kOffset32 = 0b01000000'00000000'00000000,
  };

  enum class Type {
    kObject = 0,
    kArray = 1,
    kString = 2,
    kInteger = 3,
    kFloat = 4,
    kConstant = 5,
  };

  namespace metadata {
    enum class Float {
      kPositiveZero = 0,
      kNegativeZero = 1,
    };

    enum class Constant {
      kTrue = 0,
      kFalse = 1,
      kNull = 2,
    };
  } // namespace metadata

  struct Tag {
    Type type;
    bool is_embedded;
    uint8_t metadata;
    uint32_t data;
  };

  static const size_t kRootTagSize = sizeof(uint8_t);

#ifdef GRN_WITH_SIMDJSON
  struct TagWriter {
    virtual void
    write(Type type, bool is_embedded, uint8_t metadata, uint32_t data) = 0;

  protected:
    uint32_t
    pack_tag(Type type, bool is_embedded, uint8_t metadata, uint32_t data)
    {
      uint32_t tag = static_cast<uint32_t>(type);
      if (is_embedded) {
        tag |= (1 << 3);
      }
      tag |= (metadata << 4);
      tag |= (data << 8);
      return tag;
    }
  };

  struct ContainerTagsWriter : public TagWriter {
    grn_ctx *ctx_;
    grn_obj buffer;
    uint32_t size16;
    uint32_t size32;

    ContainerTagsWriter(grn_ctx *ctx)
      : ctx_(ctx),
        buffer(),
        size16(0),
        size32(0)
    {
      GRN_BINARY_INIT(&buffer, 0);
    }

    ~ContainerTagsWriter() { GRN_OBJ_FIN(ctx_, &buffer); }

    void
    write(Type type, bool is_embedded, uint8_t metadata, uint32_t data) override
    {
      auto tag = pack_tag(type, is_embedded, metadata, data);
      if (size32 > 0 || data > 255) {
        GRN_UINT32_PUT(ctx_, &buffer, tag);
        size32 += sizeof(uint32_t);
      } else {
        GRN_UINT16_PUT(ctx_, &buffer, tag);
        size16 += sizeof(uint16_t);
      }
    }
  };

  struct RootTagWriter : public TagWriter {
    grn_ctx *ctx_;
    grn_obj *buffer_;

    RootTagWriter(grn_ctx *ctx, grn_obj *buffer) : ctx_(ctx), buffer_(buffer) {}

    ~RootTagWriter() = default;

    void
    write(Type type, bool is_embedded, uint8_t metadata, uint32_t data) override
    {
      assert(data == 0); // Root tag's data must be zero
      auto tag = pack_tag(type, is_embedded, metadata, 0);
      GRN_UINT8_PUT(ctx_, buffer_, tag);
    }
  };

  struct PositionsWriter {
    grn_ctx *ctx_;
    grn_obj buffer;
    uint32_t size8;
    uint32_t size16;
    uint32_t size32;
    uint32_t index_;
    uint32_t last_position_;

    PositionsWriter(grn_ctx *ctx)
      : ctx_(ctx),
        buffer(),
        size8(0),
        size16(0),
        size32(0),
        index_(0),
        last_position_(0)
    {
      GRN_BINARY_INIT(&buffer, 0);
    }

    ~PositionsWriter() { GRN_OBJ_FIN(ctx_, &buffer); }

    uint32_t
    write(uint32_t size)
    {
      auto index = index_;
      index_++;
      auto position = last_position_ + size;
      if (size32 > 0 || position > 65535) {
        GRN_UINT32_PUT(ctx_, &buffer, position);
        size32 += sizeof(uint32_t);
      } else if (size16 > 0 || position > 255) {
        GRN_UINT16_PUT(ctx_, &buffer, position);
        size16 += sizeof(uint16_t);
      } else {
        GRN_UINT8_PUT(ctx_, &buffer, position);
        size8 += sizeof(uint8_t);
      }
      last_position_ = position;
      return index;
    }
  };

  class JSONParser {
  public:
    JSONParser(grn_ctx *ctx, grn_obj *input, grn_obj *output)
      : ctx_(ctx),
        input_(input),
        output_(output),
        root_tag_writer_(ctx_, output),
        array_tags_writer_(ctx_),
        array_positions_writer_(ctx_)
    {
    }

    ~JSONParser() = default;

    void
    parse()
    {
      const char *tag = "[json-parser][parse]";
      auto ctx = ctx_;

      auto rc = grn_bulk_reserve(ctx_, input_, simdjson::SIMDJSON_PADDING);
      if (rc != GRN_SUCCESS) {
        return;
      }

      simdjson::ondemand::parser parser;
      simdjson::ondemand::document document;
      auto error_code = parser
                          .iterate(GRN_TEXT_VALUE(input_),
                                   GRN_TEXT_LEN(input_),
                                   GRN_BULK_WSIZE(input_))
                          .get(document);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to parse: %s",
            tag,
            simdjson::error_message(error_code));
        return;
      }

      parse_document(document);
    }

  private:
    using json_type = simdjson::ondemand::json_type;

    grn_ctx *ctx_;
    grn_obj *input_;
    grn_obj *output_;
    RootTagWriter root_tag_writer_;
    ContainerTagsWriter array_tags_writer_;
    PositionsWriter array_positions_writer_;

    void
    append_tags_buffer_offsets(uint32_t &flags,
                               uint32_t &offset,
                               std::vector<uint32_t> &buffer_offsets,
                               ContainerTagsWriter *writer,
                               Flag flag16,
                               Flag flag32)
    {
      if (writer->size16 > 0) {
        flags |= static_cast<uint32_t>(flag16);
        buffer_offsets.push_back(offset);
        offset += writer->size16;
      }
      if (writer->size32 > 0) {
        flags |= static_cast<uint32_t>(flag32);
        buffer_offsets.push_back(offset);
        offset += writer->size32;
      }
    }

    void
    append_positions_buffer_offsets(uint32_t &flags,
                                    uint32_t &offset,
                                    std::vector<uint32_t> &buffer_offsets,
                                    PositionsWriter *writer,
                                    Flag flag8,
                                    Flag flag16,
                                    Flag flag32)
    {
      if (writer->size8 > 0) {
        flags |= static_cast<uint32_t>(flag8);
        buffer_offsets.push_back(offset);
        offset += writer->size8;
      }
      if (writer->size16 > 0) {
        flags |= static_cast<uint32_t>(flag16);
        buffer_offsets.push_back(offset);
        offset += writer->size16;
      }
      if (writer->size32 > 0) {
        flags |= static_cast<uint32_t>(flag32);
        buffer_offsets.push_back(offset);
        offset += writer->size32;
      }
    }

    void
    write_footer()
    {
      uint32_t flags = 0;
      uint32_t offset = sizeof(uint32_t);
      std::vector<grn_obj *> outputs;
      std::vector<uint32_t> buffer_offsets;

      if (GRN_TEXT_LEN(&(array_positions_writer_.buffer)) > 0) {
        outputs.push_back(&(array_positions_writer_.buffer));
        append_positions_buffer_offsets(flags,
                                        offset,
                                        buffer_offsets,
                                        &array_positions_writer_,
                                        Flag::kArrayPosition8,
                                        Flag::kArrayPosition16,
                                        Flag::kArrayPosition32);
      }
      if (GRN_TEXT_LEN(&(array_tags_writer_.buffer)) > 0) {
        outputs.push_back(&(array_tags_writer_.buffer));
        append_tags_buffer_offsets(flags,
                                   offset,
                                   buffer_offsets,
                                   &array_tags_writer_,
                                   Flag::kArrayTag16,
                                   Flag::kArrayTag32);
      }

      if (!buffer_offsets.empty() && buffer_offsets.back() > 65535) {
        flags |= static_cast<uint32_t>(Flag::kOffset32);
      }

      GRN_UINT8_PUT(ctx_, output_, (flags >> 16));
      GRN_UINT8_PUT(ctx_, output_, ((flags >> 8) & 0xff));
      GRN_UINT8_PUT(ctx_, output_, (flags & 0xff));
      for (auto output : outputs) {
        GRN_TEXT_PUT(ctx_,
                     output_,
                     GRN_TEXT_VALUE(output),
                     GRN_TEXT_LEN(output));
      }
      if (flags & static_cast<uint32_t>(Flag::kOffset32)) {
        for (auto offset : buffer_offsets) {
          GRN_UINT32_PUT(ctx_, output_, offset);
        }
      } else {
        for (auto offset : buffer_offsets) {
          GRN_UINT16_PUT(ctx_, output_, offset);
        }
      }
    }

    bool
    parse_document(simdjson::ondemand::document &document)
    {
      const char *tag = "[json-parser][parse][document]";
      auto ctx = ctx_;

      json_type type;
      auto error_code = document.type().get(type);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get root value type: %s",
            tag,
            simdjson::error_message(error_code));
        return true;
      }
      switch (type) {
      case json_type::array:
        {
          simdjson::ondemand::value value;
          error_code = document.get_value().get(value);
          if (error_code != simdjson::SUCCESS) {
            ERR(GRN_INVALID_ARGUMENT,
                "%s failed to get root value: %s",
                tag,
                simdjson::error_message(error_code));
            return false;
          }
          if (!parse_container(&root_tag_writer_, value)) {
            return false;
          }
          write_footer();
        }
        return true;
      case json_type::object:
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s object isn't supported yet", tag);
        return false;
      case json_type::number:
        if (!parse_number(&root_tag_writer_, document)) {
          return false;
        }
        return true;
      case json_type::string:
        if (!parse_string(&root_tag_writer_, document)) {
          return false;
        }
        return true;
      case json_type::boolean:
        if (!parse_boolean(&root_tag_writer_, document)) {
          return false;
        }
        return true;
      case json_type::null:
        if (!parse_null(&root_tag_writer_, document)) {
          return false;
        }
        return true;
      default:
        ERR(GRN_INVALID_ARGUMENT, "%s failed to get root value", tag);
        return false;
      }
    }

    bool
    parse_value(TagWriter *tag_writer, simdjson::ondemand::value &value)
    {
      const char *tag = "[json-parser][parse][value]";
      auto ctx = ctx_;

      json_type type;
      auto error_code = value.type().get(type);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get value type: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }
      switch (type) {
      case json_type::array:
        if (!parse_container(tag_writer, value)) {
          return false;
        }
        return true;
      case json_type::object:
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s object isn't supported yet", tag);
        return false;
      case json_type::number:
        if (!parse_number(tag_writer, value)) {
          return false;
        }
        return true;
      case json_type::string:
        if (!parse_string(tag_writer, value)) {
          return false;
        }
        return true;
      case json_type::boolean:
        if (!parse_boolean(tag_writer, value)) {
          return false;
        }
        return true;
      case json_type::null:
        if (!parse_null(tag_writer, value)) {
          return false;
        }
        return true;
      default:
        ERR(GRN_INVALID_ARGUMENT, "%s failed to get value", tag);
        return false;
      }
    }

    bool
    parse_container(TagWriter *tag_writer, simdjson::ondemand::value &value)
    {
      const char *tag = "[json-parser][parse][container]";
      auto ctx = ctx_;

      // TODO: We want to use breadth-first search but it doesn't work
      // for now. simdjson::ondemand::value is available only while
      // the value is currently processed. We can't process it later
      // by keeping it in a queue.
      std::queue<simdjson::ondemand::value> values;
      std::queue<TagWriter *> tag_writers;
      values.push(value);
      tag_writers.push(tag_writer);
      while (!values.empty()) {
        auto current_value = values.front();
        values.pop();
        auto current_tag_writer = tag_writers.front();
        tag_writers.pop();

        json_type type;
        auto error_code = current_value.type().get(type);
        if (error_code != simdjson::SUCCESS) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s failed to get value type: %s",
              tag,
              simdjson::error_message(error_code));
          return false;
        }

        switch (type) {
        case json_type::array:
          {
            simdjson::ondemand::array array;
            error_code = value.get_array().get(array);
            if (error_code != simdjson::SUCCESS) {
              ERR(GRN_INVALID_ARGUMENT,
                  "%s failed to get array: %s",
                  tag,
                  simdjson::error_message(error_code));
              return false;
            }
            size_t n_elements = 0;
            for (auto element_result : array) {
              simdjson::ondemand::value element;
              error_code = element_result.get(element);
              if (error_code != simdjson::SUCCESS) {
                ERR(GRN_INVALID_ARGUMENT,
                    "%s failed to get array element: %s",
                    tag,
                    simdjson::error_message(error_code));
                return false;
              }
              values.push(element);
              tag_writers.push(&array_tags_writer_);
              n_elements++;
            }
            auto index = array_positions_writer_.write(n_elements);
            current_tag_writer->write(Type::kArray, false, 0, index);
          }
          break;
        case json_type::object:
          ERR(GRN_INVALID_ARGUMENT, "%s object isn't supported yet", tag);
          return false;
        case json_type::number:
          if (!parse_number(current_tag_writer, current_value)) {
            return false;
          }
          break;
        case json_type::string:
          ERR(GRN_INVALID_ARGUMENT,
              "%s string in container isn't supported yet",
              tag);
          return false;
        case json_type::boolean:
          if (!parse_boolean(current_tag_writer, current_value)) {
            return false;
          }
          break;
        case json_type::null:
          if (!parse_null(current_tag_writer, current_value)) {
            return false;
          }
          break;
        default:
          ERR(GRN_INVALID_ARGUMENT,
              "%s invalid value type: %u",
              tag,
              static_cast<unsigned int>(type));
          return false;
        }
      }
      return true;
    }

    template <typename ValueType>
    bool
    parse_double(TagWriter *tag_writer, ValueType &value)
    {
      auto ctx = ctx_;
      const char *tag = "[json-parser][parse][double]";

      double double_value;
      auto error_code = value.get_double().get(double_value);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get floating-point number: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }
      if (std::fpclassify(double_value) == FP_ZERO) {
        uint8_t metadata =
          std::signbit(double_value)
            ? static_cast<uint8_t>(metadata::Float::kNegativeZero)
            : static_cast<uint8_t>(metadata::Float::kPositiveZero);
        tag_writer->write(Type::kFloat, true, metadata, 0);
      } else {
        tag_writer->write(Type::kFloat, false, 0, 0);
        GRN_FLOAT_PUT(ctx_, output_, double_value);
      }
      return true;
    }

    template <typename ValueType>
    bool
    parse_int64(TagWriter *tag_writer, ValueType &value)
    {
      auto ctx = ctx_;
      const char *tag = "[json-parser][parse][int64]";

      int64_t int64_value;
      auto error_code = value.get_int64().get(int64_value);
      if (error_code != simdjson::SUCCESS) {
#  ifndef BIGINT_NUMBER
        return parse_double(tag_writer, value);
#  endif
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get int64 number: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }
      uint8_t n_bytes;
      if (std::numeric_limits<int8_t>::min() <= int64_value &&
          int64_value <= std::numeric_limits<int8_t>::max()) {
        n_bytes = 1;
      } else if (std::numeric_limits<int16_t>::min() <= int64_value &&
                 int64_value <= std::numeric_limits<int16_t>::max()) {
        n_bytes = 2;
      } else if (std::numeric_limits<int32_t>::min() <= int64_value &&
                 int64_value <= std::numeric_limits<int32_t>::max()) {
        n_bytes = 4;
      } else {
        n_bytes = 8;
      }
      tag_writer->write(Type::kInteger, false, n_bytes, 0);
      if (n_bytes == 1) {
        GRN_INT8_PUT(ctx_, output_, int64_value);
      } else if (n_bytes == 2) {
        GRN_INT16_PUT(ctx_, output_, int64_value);
      } else if (n_bytes == 4) {
        GRN_INT32_PUT(ctx_, output_, int64_value);
      } else {
        GRN_INT64_PUT(ctx_, output_, int64_value);
      }
      return true;
    }

    template <typename ValueType>
    bool
    parse_number(TagWriter *tag_writer, ValueType &value)
    {
      using number_type = simdjson::ondemand::number_type;
      const char *tag = "[json-parser][parse][number]";
      auto ctx = ctx_;

      number_type type;
      auto error_code = value.get_number_type().get(type);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get number type: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }

      switch (type) {
      case number_type::floating_point_number:
#  ifdef BIGINT_NUMBER
      case number_type::big_integer: // simdjson 3.7.1 or later has this.
#  endif
        return parse_double(tag_writer, value);
        break;
      case number_type::unsigned_integer:
        // simdjson < 3.1.0 detects -9223372036854775808 as unsigned
        // integer: https://github.com/simdjson/simdjson/pull/1819
        if constexpr (simdjson::SIMDJSON_VERSION_MAJOR < 3 ||
                      (simdjson::SIMDJSON_VERSION_MAJOR == 3 &&
                       simdjson::SIMDJSON_VERSION_MINOR < 1)) {
          // Try parsing this as an int64 value. parse_int64() falls
          // back to parse_double() when this is not an int64 value.
          return parse_int64(tag_writer, value);
        }
        return parse_double(tag_writer, value);
        break;
      case number_type::signed_integer:
        return parse_int64(tag_writer, value);
      default:
        ERR(GRN_INVALID_ARGUMENT,
            "%s unsupported number type: %d",
            tag,
            static_cast<int32_t>(type));
        return false;
      }
    }

    template <typename ValueType>
    bool
    parse_string(TagWriter *tag_writer, ValueType &value)
    {
      const char *tag = "[json-parser][parse][string]";
      auto ctx = ctx_;

      std::string_view string_value;
      auto error_code = value.get_string().get(string_value);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get value: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }

      tag_writer->write(Type::kString, false, 0, 0);
      GRN_TEXT_PUT(ctx_, output_, string_value.data(), string_value.size());
      return true;
    }

    template <typename ValueType>
    bool
    parse_boolean(TagWriter *tag_writer, ValueType &value)
    {
      const char *tag = "[json-parser][parse][boolean]";
      auto ctx = ctx_;

      bool bool_value;
      auto error_code = value.get_bool().get(bool_value);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get value: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }
      uint8_t metadata = bool_value
                           ? static_cast<uint8_t>(metadata::Constant::kTrue)
                           : static_cast<uint8_t>(metadata::Constant::kFalse);
      tag_writer->write(Type::kConstant, true, metadata, 0);
      return true;
    }

    template <typename ValueType>
    bool
    parse_null(TagWriter *tag_writer, ValueType &value)
    {
      const char *tag = "[json-parser][parse][null]";
      auto ctx = ctx_;

      bool is_null;
      if constexpr (simdjson::SIMDJSON_VERSION_MAJOR >= 4 ||
                    (simdjson::SIMDJSON_VERSION_MAJOR == 3 &&
                     simdjson::SIMDJSON_VERSION_MINOR >= 1)) {
        auto error_code = value.is_null().get(is_null);
        if (error_code != simdjson::SUCCESS) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s failed to determine null: %s",
              tag,
              simdjson::error_message(error_code));
          return false;
        }
      } else {
        is_null = value.is_null();
      }
      uint8_t metadata = static_cast<uint8_t>(metadata::Constant::kNull);
      tag_writer->write(Type::kConstant, true, metadata, 0);
      return true;
    }
  };
#endif

  class JSONDataReader {
  public:
    JSONDataReader(grn_obj *json) : json_(json) {}
    ~JSONDataReader() = default;

  protected:
    grn_obj *json_;

    template <typename ValueType>
    ValueType
    read_data(uint32_t offset)
    {
      return *reinterpret_cast<const ValueType *>(GRN_JSON_VALUE(json_) +
                                                  offset);
    }
  };

  class VariableSizeTagResolver : JSONDataReader {
  public:
    VariableSizeTagResolver(grn_obj *json,
                            uint32_t offset16,
                            uint32_t n16,
                            uint32_t offset32)
      : JSONDataReader(json),
        offset16_(offset16),
        n16_(n16),
        offset32_(offset32)
    {
    }
    ~VariableSizeTagResolver() = default;

    uint32_t
    resolve(uint32_t i)
    {
      if (i < n16_) {
        auto offset = offset16_ + (sizeof(uint16_t) * i);
        return read_data<uint16_t>(offset);
      } else {
        auto offset = offset32_ + (sizeof(uint32_t) * (i - n16_));
        return read_data<uint32_t>(offset);
      }
    }

  private:
    uint32_t offset16_;
    uint32_t n16_;
    uint32_t offset32_;
  };

  class VariableSizePositionResolver : JSONDataReader {
  public:
    VariableSizePositionResolver(grn_obj *json,
                                 uint32_t offset8,
                                 uint32_t n8,
                                 uint32_t offset16,
                                 uint32_t n16,
                                 uint32_t offset32)
      : JSONDataReader(json),
        offset8_(offset8),
        n8_(n8),
        offset16_(offset16),
        n16_(n16),
        offset32_(offset32)
    {
    }
    ~VariableSizePositionResolver() = default;

    std::pair<uint32_t, uint32_t>
    resolve(uint32_t i)
    {
      uint32_t start = 0;
      uint32_t next_start = 0;
      if (i < n8_) {
        auto offset = offset8_ + (sizeof(uint8_t) * i);
        if (i == 0) {
          start = 0;
          next_start = read_data<uint8_t>(offset);
        } else {
          auto previous_offset = offset - sizeof(uint8_t);
          start = read_data<uint8_t>(previous_offset);
          next_start = read_data<uint8_t>(offset);
        }
      } else if (i < (n8_ + n16_)) {
        auto offset = offset16_ + (sizeof(uint16_t) * (i - n8_));
        if (i == 0) {
          start = 0;
          next_start = read_data<uint16_t>(offset);
        } else if (i == n8_) {
          auto last_offset8_offset = offset8_ + (sizeof(uint8_t) * (n8_ - 1));
          start = read_data<uint8_t>(last_offset8_offset);
          next_start = read_data<uint16_t>(offset);
        } else {
          auto previous_offset = offset - sizeof(uint16_t);
          start = read_data<int16_t>(previous_offset);
          next_start = read_data<uint16_t>(offset);
        }
      } else {
        auto offset = offset32_ + (sizeof(uint32_t) * (i - n8_ - n16_));
        if (i == 0) {
          start = 0;
          next_start = read_data<uint32_t>(offset);
        } else if (i == (n8_ + n16_)) {
          if (n16_ == 0) {
            if (n8_ == 0) {
              start = 0;
            } else {
              auto last_offset8_offset =
                offset8_ + (sizeof(uint8_t) * (n8_ - 1));
              start = read_data<uint8_t>(last_offset8_offset);
            }
          } else {
            auto last_offset16_offset =
              offset16_ + (sizeof(uint16_t) * (n16_ - 1));
            start = read_data<uint16_t>(last_offset16_offset);
          }
          next_start = read_data<uint32_t>(offset);
        } else {
          auto previous_offset = offset - sizeof(uint32_t);
          start = read_data<uint32_t>(previous_offset);
          next_start = read_data<uint32_t>(offset);
        }
      }
      return {start, next_start};
    }

  private:
    uint32_t offset8_;
    uint32_t n8_;
    uint32_t offset16_;
    uint32_t n16_;
    uint32_t offset32_;
  };

  class JSONReader : JSONDataReader {
  public:
    JSONReader(grn_ctx *ctx, grn_obj *json)
      : JSONDataReader(json),
        ctx_(ctx),
        flags_(0),
        array_position_resolver_(),
        array_tag_resolver_(),
        states_(),
        current_type_(GRN_JSON_VALUE_UNKNOWN),
        current_value_(nullptr),
        current_size_(0)
    {
      // Root tag exists.
      if (GRN_JSON_LEN(json_) >= sizeof(uint8_t)) {
        states_.emplace(GRN_JSON_VALUE_UNKNOWN, 0, 0);
      }
      GRN_VOID_INIT(&null_value_);
      GRN_BOOL_INIT(&bool_value_, 0);
      GRN_INT64_INIT(&int64_value_, 0);
      GRN_FLOAT_INIT(&float_value_, 0);
      GRN_TEXT_INIT(&string_value_, GRN_OBJ_DO_SHALLOW_COPY);
    }

    ~JSONReader()
    {
      GRN_OBJ_FIN(ctx_, &null_value_);
      GRN_OBJ_FIN(ctx_, &bool_value_);
      GRN_OBJ_FIN(ctx_, &int64_value_);
      GRN_OBJ_FIN(ctx_, &float_value_);
      GRN_OBJ_FIN(ctx_, &string_value_);
    };

    grn_rc
    next()
    {
      const char *log_tag = "[json-reader][next]";
      auto ctx = ctx_;

      if (states_.empty()) {
        return GRN_END_OF_DATA;
      }
      auto &state = states_.top();
      if (state.type == GRN_JSON_VALUE_UNKNOWN) {
        const bool is_root = true;
        states_.pop();
        auto tag = unpack_tag(read_data<uint8_t>(0));
        return next_value(tag, is_root);
      } else if (state.type == GRN_JSON_VALUE_ARRAY) {
        auto raw_tag = array_tag_resolver_->resolve(state.index);
        if (state.index == state.last_index) {
          states_.pop();
        } else {
          state.index++;
        }
        auto tag = unpack_tag(raw_tag);
        return next_value(tag, false);
      } else {
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
            "%s unsupported value type: %u",
            log_tag,
            static_cast<uint32_t>(state.type));
        return ctx_->rc;
      }
    }

    grn_json_value_type
    type()
    {
      return current_type_;
    }

    grn_obj *
    value()
    {
      return current_value_;
    }

    size_t
    size()
    {
      return current_size_;
    }

  private:
    struct State {
      grn_json_value_type type;
      uint32_t index;
      uint32_t last_index;

      State(grn_json_value_type type, uint32_t index, uint32_t last_index)
        : type(type),
          index(index),
          last_index(last_index)
      {
      }
    };

    grn_ctx *ctx_;
    uint32_t flags_;
    std::unique_ptr<VariableSizePositionResolver> array_position_resolver_;
    std::unique_ptr<VariableSizeTagResolver> array_tag_resolver_;
    std::stack<State> states_;
    grn_json_value_type current_type_;
    grn_obj *current_value_;
    size_t current_size_;
    grn_obj null_value_;
    grn_obj bool_value_;
    grn_obj int64_value_;
    grn_obj float_value_;
    grn_obj string_value_;

    Tag
    unpack_tag(uint32_t raw_tag)
    {
      Tag tag;
      tag.type = static_cast<Type>(raw_tag & 0b111);
      tag.is_embedded = (raw_tag & (0b1000));
      tag.metadata = ((raw_tag >> 4) & (0b00001111));
      tag.data = (raw_tag >> 8);
      return tag;
    }

    std::unique_ptr<VariableSizeTagResolver>
    create_variable_size_tag_resolver(
      uint32_t &i,
      const std::vector<uint32_t> &buffer_offsets,
      Flag flag16,
      Flag flag32)
    {
      uint32_t offset16 = 0;
      uint32_t n16 = 0;
      if (flags_ & static_cast<uint32_t>(flag16)) {
        offset16 = buffer_offsets[i];
        n16 = (buffer_offsets[i + 1] - buffer_offsets[i]) / sizeof(uint16_t);
        i++;
      }

      uint32_t offset32 = 0;
      if (flags_ & static_cast<uint32_t>(flag32)) {
        offset32 = buffer_offsets[i];
        i++;
      }

      return std::make_unique<VariableSizeTagResolver>(json_,
                                                       offset16,
                                                       n16,
                                                       offset32);
    }

    std::unique_ptr<VariableSizePositionResolver>
    create_variable_size_position_resolver(
      uint32_t &i,
      const std::vector<uint32_t> &buffer_offsets,
      Flag flag8,
      Flag flag16,
      Flag flag32)
    {
      uint32_t offset8 = 0;
      uint32_t n8 = 0;
      if (flags_ & static_cast<uint32_t>(flag8)) {
        offset8 = buffer_offsets[i];
        n8 = (buffer_offsets[i + 1] - buffer_offsets[i]) / sizeof(uint8_t);
        i++;
      }

      uint32_t offset16 = 0;
      uint32_t n16 = 0;
      if (flags_ & static_cast<uint32_t>(flag16)) {
        offset16 = buffer_offsets[i];
        n16 = (buffer_offsets[i + 1] - buffer_offsets[i]) / sizeof(uint16_t);
        i++;
      }

      uint32_t offset32 = 0;
      if (flags_ & static_cast<uint32_t>(flag32)) {
        offset32 = buffer_offsets[i];
        i++;
      }

      return std::make_unique<VariableSizePositionResolver>(json_,
                                                            offset8,
                                                            n8,
                                                            offset16,
                                                            n16,
                                                            offset32);
    }

    grn_rc
    read_footer()
    {
      uint32_t flags_offset = kRootTagSize;
      flags_ |= read_data<uint8_t>(flags_offset) << 16;
      flags_ |= read_data<uint8_t>(flags_offset + sizeof(uint8_t)) << 8;
      flags_ |= read_data<uint8_t>(flags_offset + sizeof(uint8_t) * 2);

      uint32_t n_buffer_offsets = 0;
      if (flags_ & static_cast<uint32_t>(Flag::kObjectPosition8)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kObjectPosition16)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kObjectPosition32)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kObjectKey)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kObjectKeyPosition8)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kObjectKeyPosition16)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kObjectKeyPosition32)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kObjectValueTag16)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kObjectValueTag32)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kArrayPosition8)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kArrayPosition16)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kArrayPosition32)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kArrayTag16)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kArrayTag32)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kStringValue)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kStringPosition8)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kStringPosition16)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kStringPosition32)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kInt16)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kInt32)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kInt64)) {
        n_buffer_offsets++;
      }
      if (flags_ & static_cast<uint32_t>(Flag::kDouble)) {
        n_buffer_offsets++;
      }

      std::vector<uint32_t> buffer_offsets;
      if (flags_ & static_cast<uint32_t>(Flag::kOffset32)) {
        uint32_t offset =
          GRN_JSON_LEN(json_) - (sizeof(uint32_t) * n_buffer_offsets);
        for (uint32_t i = 0; i < n_buffer_offsets; ++i) {
          buffer_offsets.push_back(read_data<uint32_t>(offset));
          offset += sizeof(uint32_t);
        }
      } else {
        uint32_t offset =
          GRN_JSON_LEN(json_) - (sizeof(uint16_t) * n_buffer_offsets);
        for (uint32_t i = 0; i < n_buffer_offsets; ++i) {
          buffer_offsets.push_back(read_data<uint16_t>(offset));
          offset += sizeof(uint16_t);
        }
      }
      buffer_offsets.push_back(GRN_JSON_LEN(json_));

      uint32_t i = 0;
      array_position_resolver_ =
        create_variable_size_position_resolver(i,
                                               buffer_offsets,
                                               Flag::kArrayPosition8,
                                               Flag::kArrayPosition16,
                                               Flag::kArrayPosition32);
      array_tag_resolver_ =
        create_variable_size_tag_resolver(i,
                                          buffer_offsets,
                                          Flag::kArrayTag16,
                                          Flag::kArrayTag32);
      return GRN_SUCCESS;
    }

    grn_rc
    next_value(const Tag &tag, bool is_root)
    {
      const char *log_tag = "[json-reader][next][value]";
      auto ctx = ctx_;

      if (tag.type == Type::kConstant) {
        return next_constant(tag);
      }

      if (is_root && (tag.type == Type::kArray || tag.type == Type::kObject)) {
        auto rc = read_footer();
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }

      switch (tag.type) {
      case Type::kArray:
        return next_array(tag, is_root);
      case Type::kInteger:
        return next_integer(tag, is_root);
      case Type::kFloat:
        return next_float(tag, is_root);
      case Type::kString:
        return next_string(tag, is_root);
      default:
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
            "%s unsupported root value type",
            log_tag);
        return ctx->rc;
      }
    }

    grn_rc
    next_array(const Tag &tag, bool is_root)
    {
      auto index = tag.data;
      auto resolved = array_position_resolver_->resolve(index);
      auto start = resolved.first;
      auto next_start = resolved.second;
      current_type_ = GRN_JSON_VALUE_ARRAY;
      current_value_ = nullptr;
      current_size_ = next_start - start;
      if (current_size_ > 0) {
        states_.emplace(GRN_JSON_VALUE_ARRAY, start, next_start - 1);
      }
      return GRN_SUCCESS;
    }

    grn_rc
    next_float(const Tag &tag, bool is_root)
    {
      const char *log_tag = "[json-reader][next][float]";
      auto ctx = ctx_;

      if (tag.is_embedded) {
        switch (static_cast<metadata::Float>(tag.metadata)) {
        case metadata::Float::kPositiveZero:
          GRN_FLOAT_SET(ctx_, &float_value_, 0.0);
          break;
        case metadata::Float::kNegativeZero:
          GRN_FLOAT_SET(ctx_, &float_value_, -0.0);
          break;
        default:
          ERR(GRN_INVALID_ARGUMENT,
              "%s unknown embedded float: %u",
              log_tag,
              tag.metadata);
          return ctx->rc;
        }
        current_type_ = GRN_JSON_VALUE_FLOAT;
        current_value_ = &float_value_;
        current_size_ = 0;
      } else {
        if (is_root) {
          uint32_t offset = kRootTagSize;
          size_t size = GRN_JSON_LEN(json_) - offset;
          if (size != sizeof(double)) {
            ERR(GRN_INVALID_ARGUMENT,
                "%s data size mismatch: expected:%" GRN_FMT_SIZE
                " actual:%" GRN_FMT_SIZE,
                log_tag,
                sizeof(double),
                size);
            return ctx->rc;
          }
          auto value = read_data<double>(offset);
          current_type_ = GRN_JSON_VALUE_FLOAT;
          GRN_FLOAT_SET(ctx_, &float_value_, value);
          current_value_ = &float_value_;
          current_size_ = 0;
        } else {
          ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
              "%s root value is only supported",
              log_tag);
          return ctx->rc;
        }
      }
      return ctx->rc;
    }

    grn_rc
    next_integer(const Tag &tag, bool is_root)
    {
      const char *log_tag = "[json-reader][next][integer]";
      auto ctx = ctx_;

      if (is_root) {
        uint8_t n_bytes = tag.metadata;
        uint32_t offset = kRootTagSize;
        size_t size = GRN_JSON_LEN(json_) - offset;
        if (size != n_bytes) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s data size mismatch: expected:%u actual:%" GRN_FMT_SIZE,
              log_tag,
              n_bytes,
              size);
          return ctx->rc;
        }
        int64_t value;
        if (n_bytes == 1) {
          value = read_data<int8_t>(offset);
        } else if (n_bytes == 2) {
          value = read_data<int16_t>(offset);
        } else if (n_bytes == 4) {
          value = read_data<int32_t>(offset);
        } else {
          value = read_data<int64_t>(offset);
        }
        current_type_ = GRN_JSON_VALUE_INT64;
        GRN_INT64_SET(ctx_, &int64_value_, value);
        current_value_ = &int64_value_;
        current_size_ = 0;
      } else {
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
            "%s root value is only supported",
            log_tag);
        return ctx->rc;
      }
      return ctx->rc;
    }

    grn_rc
    next_string(const Tag &tag, bool is_root)
    {
      const char *log_tag = "[json-reader][next][string]";
      auto ctx = ctx_;

      if (is_root) {
        uint32_t offset = kRootTagSize;
        size_t size = GRN_JSON_LEN(json_) - offset;
        current_type_ = GRN_JSON_VALUE_STRING;
        GRN_TEXT_SET(ctx_,
                     &string_value_,
                     GRN_JSON_VALUE(json_) + offset,
                     size);
        current_value_ = &string_value_;
        current_size_ = 0;
      } else {
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
            "%s root value is only supported",
            log_tag);
        return ctx->rc;
      }
      return ctx->rc;
    }

    grn_rc
    next_constant(const Tag &tag)
    {
      const char *log_tag = "[json-reader][next][constant]";
      auto ctx = ctx_;

      current_size_ = 0;
      auto constant_metadata = static_cast<metadata::Constant>(tag.metadata);
      switch (constant_metadata) {
      case metadata::Constant::kTrue:
        current_type_ = GRN_JSON_VALUE_TRUE;
        GRN_BOOL_SET(ctx_, &bool_value_, true);
        current_value_ = &bool_value_;
        current_size_ = 0;
        break;
      case metadata::Constant::kFalse:
        current_type_ = GRN_JSON_VALUE_FALSE;
        GRN_BOOL_SET(ctx_, &bool_value_, false);
        current_value_ = &bool_value_;
        current_size_ = 0;
        break;
      case metadata::Constant::kNull:
        current_type_ = GRN_JSON_VALUE_NULL;
        current_value_ = &null_value_;
        break;
      default:
        ERR(GRN_INVALID_ARGUMENT,
            "%s invalid constant metadata: %u",
            log_tag,
            tag.metadata);
        return ctx->rc;
      }
      return ctx->rc;
    }
  };

  class JSONStringifier {
  public:
    JSONStringifier(grn_ctx *ctx, grn_obj *json, grn_obj *buffer)
      : ctx_(ctx),
        json_(json),
        buffer_(buffer)
    {
    }
    ~JSONStringifier() = default;

    void
    stringify()
    {
      JSONReader reader(ctx_, json_);
      while (true) {
        if (!stringify_value(reader)) {
          break;
        }
      }
    }

  private:
    grn_ctx *ctx_;
    grn_obj *json_;
    grn_obj *buffer_;

    bool
    stringify_value(JSONReader &reader)
    {
      const char *tag = "[json][to-string]";

      auto rc = reader.next();
      if (rc == GRN_END_OF_DATA) {
        return false;
      }
      if (rc != GRN_SUCCESS) {
        return false;
      }
      switch (reader.type()) {
      case GRN_JSON_VALUE_NULL:
        GRN_TEXT_PUTS(ctx_, buffer_, "null");
        return true;
      case GRN_JSON_VALUE_FALSE:
        GRN_TEXT_PUTS(ctx_, buffer_, "false");
        return true;
      case GRN_JSON_VALUE_TRUE:
        GRN_TEXT_PUTS(ctx_, buffer_, "true");
        return true;
      case GRN_JSON_VALUE_INT64:
        grn_text_otoj(ctx_, buffer_, reader.value(), nullptr);
        return true;
      case GRN_JSON_VALUE_FLOAT:
        grn_text_otoj(ctx_, buffer_, reader.value(), nullptr);
        return true;
      case GRN_JSON_VALUE_STRING:
        grn_text_otoj(ctx_, buffer_, reader.value(), nullptr);
        return true;
      case GRN_JSON_VALUE_ARRAY:
        {
          GRN_TEXT_PUTS(ctx_, buffer_, "[");
          size_t n = reader.size();
          for (size_t i = 0; i < n; ++i) {
            if (i != 0) {
              GRN_TEXT_PUTS(ctx_, buffer_, ",");
            }
            if (!stringify_value(reader)) {
              return false;
            }
          }
          GRN_TEXT_PUTS(ctx_, buffer_, "]");
          return true;
        }
      case GRN_JSON_VALUE_OBJECT:
        {
          GRN_TEXT_PUTS(ctx_, buffer_, "{");
          size_t n = reader.size();
          for (size_t i = 0; i < n; ++i) {
            if (i != 0) {
              GRN_TEXT_PUTS(ctx_, buffer_, ",");
            }
            if (!stringify_value(reader)) {
              return false;
            }
            GRN_TEXT_PUTS(ctx_, buffer_, ":");
            if (!stringify_value(reader)) {
              return false;
            }
          }
          GRN_TEXT_PUTS(ctx_, buffer_, "}");
          return true;
        }
      default:
        {
          auto ctx = ctx_;
          ERR(GRN_INVALID_ARGUMENT, "%s invalid JSON", tag);
          return false;
        }
      }
    }
  };
} // namespace

extern "C" {
struct _grn_json_parser {
#ifdef GRN_WITH_SIMDJSON
  JSONParser *parser;
#else
  void *parser;
#endif
};

grn_json_parser *
grn_json_parser_open(grn_ctx *ctx, grn_obj *input, grn_obj *output)
{
  GRN_API_ENTER;
  grn_json_parser *parser = nullptr;
#ifdef GRN_WITH_SIMDJSON
  parser = static_cast<grn_json_parser *>(GRN_MALLOC(sizeof(grn_json_parser)));
  if (!parser) {
    GRN_API_RETURN(nullptr);
  }
  parser->parser = new JSONParser(ctx, input, output);
#else
  const char *tag = "[json-parser][open]";
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s simdjson isn't enabled", tag);
#endif
  GRN_API_RETURN(parser);
}

grn_rc
grn_json_parser_close(grn_ctx *ctx, grn_json_parser *parser)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_SIMDJSON
  if (parser) {
    delete parser->parser;
    GRN_FREE(parser);
  }
#endif
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_json_parser_parse(grn_ctx *ctx, grn_json_parser *parser)
{
  GRN_API_ENTER;
#ifdef GRN_WITH_SIMDJSON
  parser->parser->parse();
#else
  const char *tag = "[json-parser][parse]";
  ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s simdjson isn't enabled", tag);
#endif
  GRN_API_RETURN(ctx->rc);
}

struct _grn_json_reader {
  JSONReader *reader;
};

grn_json_reader *
grn_json_reader_open(grn_ctx *ctx, grn_obj *json)
{
  GRN_API_ENTER;
  auto reader =
    static_cast<grn_json_reader *>(GRN_MALLOC(sizeof(grn_json_reader)));
  if (!reader) {
    GRN_API_RETURN(nullptr);
  }
  reader->reader = new JSONReader(ctx, json);
  GRN_API_RETURN(reader);
}

grn_rc
grn_json_reader_close(grn_ctx *ctx, grn_json_reader *reader)
{
  GRN_API_ENTER;
  if (reader) {
    delete reader->reader;
    GRN_FREE(reader);
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_json_reader_next(grn_ctx *ctx, grn_json_reader *reader)
{
  GRN_API_ENTER;
  reader->reader->next();
  GRN_API_RETURN(ctx->rc);
}

grn_json_value_type
grn_json_reader_get_type(grn_ctx *ctx, grn_json_reader *reader)
{
  GRN_API_ENTER;
  grn_json_value_type type = reader->reader->type();
  GRN_API_RETURN(type);
}

grn_obj *
grn_json_reader_get_value(grn_ctx *ctx, grn_json_reader *reader)
{
  GRN_API_ENTER;
  grn_obj *value = reader->reader->value();
  GRN_API_RETURN(value);
}

size_t
grn_json_reader_get_size(grn_ctx *ctx, grn_json_reader *reader)
{
  GRN_API_ENTER;
  size_t size = reader->reader->size();
  GRN_API_RETURN(size);
}

grn_rc
grn_json_to_string(grn_ctx *ctx, grn_obj *json, grn_obj *buffer)
{
  GRN_API_ENTER;
  JSONStringifier stringifier(ctx, json, buffer);
  stringifier.stringify();
  GRN_API_RETURN(ctx->rc);
}
}
