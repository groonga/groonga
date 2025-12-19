/*
  Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>

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
#  include <string>

#  include <simdjson.h>
#endif

namespace {
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

#ifdef GRN_WITH_SIMDJSON
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

  struct TagWriter {
    grn_ctx *ctx_;
    grn_obj buffer;
    uint32_t size16;
    uint32_t size32;

    TagWriter(grn_ctx *ctx) : ctx_(ctx), buffer(), size16(0), size32(0)
    {
      GRN_BINARY_INIT(&buffer, 0);
    }

    ~TagWriter() { GRN_OBJ_FIN(ctx_, &buffer); }

    void
    write(Type type, bool is_embedded, uint8_t metadata, uint32_t data)
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

  struct RootTagWriter {
    grn_ctx *ctx_;
    grn_obj *buffer_;

    RootTagWriter(grn_ctx *ctx, grn_obj *buffer) : ctx_(ctx), buffer_(buffer) {}

    ~RootTagWriter() = default;

    void
    write(Type type, bool is_embedded, uint8_t metadata, uint32_t data)
    {
      assert(data == 0); // Root tag's data must be zero
      auto tag = pack_tag(type, is_embedded, metadata, 0);
      GRN_UINT8_PUT(ctx_, buffer_, tag);
    }
  };

  class JSONParser {
  public:
    JSONParser(grn_ctx *ctx, grn_obj *input, grn_obj *output)
      : ctx_(ctx),
        input_(input),
        output_(output),
        root_tag_writer_(ctx_, output)
    {
    }

    ~JSONParser() = default;

    void
    parse()
    {
      using json_type = simdjson::ondemand::json_type;
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

      json_type type;
      error_code = document.type().get(type);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get root value type: %s",
            tag,
            simdjson::error_message(error_code));
        return;
      }
      switch (type) {
      case json_type::array:
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s array isn't supported yet", tag);
        return;
      case json_type::object:
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s object isn't supported yet", tag);
        return;
      case json_type::number:
        if (!parse_number(root_tag_writer_, document)) {
          return;
        }
        break;
      case json_type::string:
        if (!parse_string(root_tag_writer_, document)) {
          return;
        }
        break;
      case json_type::boolean:
        if (!parse_boolean(root_tag_writer_, document)) {
          return;
        }
        break;
      case json_type::null:
        if (!parse_null(root_tag_writer_, document)) {
          return;
        }
        break;
      default:
        ERR(GRN_INVALID_ARGUMENT, "%s failed to get root value", tag);
        return;
      }
    }

  private:
    grn_ctx *ctx_;
    grn_obj *input_;
    grn_obj *output_;
    RootTagWriter root_tag_writer_;

    bool
    parse_double(RootTagWriter &root_tag_writer,
                 simdjson::ondemand::document &container)
    {
      auto ctx = ctx_;
      const char *tag = "[json-parser][parse][double]";

      double value;
      auto error_code = container.get_double().get(value);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get floating-point number: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }
      if (std::fpclassify(value) == FP_ZERO) {
        uint8_t metadata =
          std::signbit(value)
            ? static_cast<uint8_t>(metadata::Float::kNegativeZero)
            : static_cast<uint8_t>(metadata::Float::kPositiveZero);
        root_tag_writer.write(Type::kFloat, true, metadata, 0);
      } else {
        root_tag_writer.write(Type::kFloat, false, 0, 0);
        GRN_FLOAT_PUT(ctx_, output_, value);
      }
      return true;
    }

    bool
    parse_int64(RootTagWriter &root_tag_writer,
                simdjson::ondemand::document &container)
    {
      auto ctx = ctx_;
      const char *tag = "[json-parser][parse][int64]";

      int64_t value;
      auto error_code = container.get_int64().get(value);
      if (error_code != simdjson::SUCCESS) {
#  ifndef BIGINT_NUMBER
        return parse_double(root_tag_writer, container);
#  endif
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get int64 number: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }
      uint8_t n_bytes;
      if (std::numeric_limits<int8_t>::min() <= value &&
          value <= std::numeric_limits<int8_t>::max()) {
        n_bytes = 1;
      } else if (std::numeric_limits<int16_t>::min() <= value &&
                 value <= std::numeric_limits<int16_t>::max()) {
        n_bytes = 2;
      } else if (std::numeric_limits<int32_t>::min() <= value &&
                 value <= std::numeric_limits<int32_t>::max()) {
        n_bytes = 4;
      } else {
        n_bytes = 8;
      }
      root_tag_writer.write(Type::kInteger, false, n_bytes, 0);
      if (n_bytes == 1) {
        GRN_INT8_PUT(ctx_, output_, value);
      } else if (n_bytes == 2) {
        GRN_INT16_PUT(ctx_, output_, value);
      } else if (n_bytes == 4) {
        GRN_INT32_PUT(ctx_, output_, value);
      } else {
        GRN_INT64_PUT(ctx_, output_, value);
      }
      return true;
    }

    bool
    parse_number(RootTagWriter &root_tag_writer,
                 simdjson::ondemand::document &container)
    {
      using number_type = simdjson::ondemand::number_type;
      const char *tag = "[json-parser][parse][number]";
      auto ctx = ctx_;

      number_type type;
      auto error_code = container.get_number_type().get(type);
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
        return parse_double(root_tag_writer, container);
        break;
      case number_type::unsigned_integer:
        // simdjson < 3.1.0 detects -9223372036854775808 as unsigned
        // integer: https://github.com/simdjson/simdjson/pull/1819
        if constexpr (simdjson::SIMDJSON_VERSION_MAJOR < 3 ||
                      (simdjson::SIMDJSON_VERSION_MAJOR == 3 &&
                       simdjson::SIMDJSON_VERSION_MINOR < 1)) {
          // Try parsing this as an int64 value. parse_int64() falls
          // back to parse_double() when this is not an int64 value.
          return parse_int64(root_tag_writer, container);
        }
        return parse_double(root_tag_writer, container);
        break;
      case number_type::signed_integer:
        return parse_int64(root_tag_writer, container);
      default:
        ERR(GRN_INVALID_ARGUMENT,
            "%s unsupported number type: %d",
            tag,
            static_cast<int32_t>(type));
        return false;
      }
    }

    bool
    parse_string(RootTagWriter &root_tag_writer,
                 simdjson::ondemand::document &container)
    {
      const char *tag = "[json-parser][parse][string]";
      auto ctx = ctx_;

      std::string_view value;
      auto error_code = container.get_string().get(value);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get value: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }

      root_tag_writer.write(Type::kString, false, 0, 0);
      GRN_TEXT_PUT(ctx_, output_, value.data(), value.size());
      return true;
    }

    template <typename TagWriter, typename Container>
    bool
    parse_boolean(TagWriter &tag_writer, Container &container)
    {
      const char *tag = "[json-parser][parse][boolean]";
      auto ctx = ctx_;

      bool value;
      auto error_code = container.get_bool().get(value);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get value: %s",
            tag,
            simdjson::error_message(error_code));
        return false;
      }
      uint8_t metadata = value
                           ? static_cast<uint8_t>(metadata::Constant::kTrue)
                           : static_cast<uint8_t>(metadata::Constant::kFalse);
      tag_writer.write(Type::kConstant, true, metadata, 0);
      return true;
    }

    template <typename TagWriter, typename Container>
    bool
    parse_null(TagWriter &tag_writer, Container &container)
    {
      const char *tag = "[json-parser][parse][null]";
      auto ctx = ctx_;

      bool is_null;
      if constexpr (simdjson::SIMDJSON_VERSION_MAJOR >= 4 ||
                    (simdjson::SIMDJSON_VERSION_MAJOR == 3 &&
                     simdjson::SIMDJSON_VERSION_MINOR >= 1)) {
        auto error_code = container.is_null().get(is_null);
        if (error_code != simdjson::SUCCESS) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s failed to determine null: %s",
              tag,
              simdjson::error_message(error_code));
          return false;
        }
      } else {
        is_null = container.is_null();
      }
      uint8_t metadata = static_cast<uint8_t>(metadata::Constant::kNull);
      tag_writer.write(Type::kConstant, true, metadata, 0);
      return true;
    }
  };
#endif

  class JSONReader {
  public:
    JSONReader(grn_ctx *ctx, grn_obj *json)
      : ctx_(ctx),
        json_(json),
        current_offset_(0),
        current_type_(GRN_JSON_VALUE_UNKNOWN),
        current_value_(nullptr),
        current_size_(0)
    {
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
      static const char *tag = "[json-reader][next]";
      auto ctx = ctx_;

      if (current_offset_ >= GRN_JSON_LEN(json_)) {
        return GRN_END_OF_DATA;
      }
      if (current_offset_ == 0) {
        auto root_tag = unpack_tag(GRN_JSON_VALUE(json_)[current_offset_]);
        current_offset_ += sizeof(uint8_t);
        switch (root_tag.type) {
        case Type::kInteger:
          return next_integer(root_tag, true);
          break;
        case Type::kFloat:
          return next_float(root_tag, true);
          break;
        case Type::kString:
          return next_string(root_tag, true);
          break;
        case Type::kConstant:
          return next_constant(root_tag);
          break;
        default:
          ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
              "%s unsupported root value type",
              tag);
          return ctx->rc;
        }
      } else {
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
            "%s root value is only supported",
            tag);
        return ctx->rc;
      }
      return ctx->rc;
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
    grn_ctx *ctx_;
    grn_obj *json_;
    uint32_t current_offset_;
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

    grn_rc
    next_float(const Tag &tag, bool is_root)
    {
      auto ctx = ctx_;
      static const char *log_tag = "[json-reader][next][float]";

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
          size_t size = GRN_JSON_LEN(json_) - current_offset_;
          if (size != sizeof(double)) {
            ERR(GRN_INVALID_ARGUMENT,
                "%s data size mismatch: expected:%" GRN_FMT_SIZE
                " actual:%" GRN_FMT_SIZE,
                log_tag,
                sizeof(double),
                size);
            return ctx->rc;
          }
          double value = reinterpret_cast<const double *>(
            GRN_JSON_VALUE(json_) + current_offset_)[0];
          current_type_ = GRN_JSON_VALUE_FLOAT;
          GRN_FLOAT_SET(ctx_, &float_value_, value);
          current_value_ = &float_value_;
          current_size_ = 0;
          current_offset_ += size;
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
      auto ctx = ctx_;
      static const char *log_tag = "[json-reader][next][integer]";

      if (is_root) {
        uint8_t n_bytes = tag.metadata;
        size_t size = GRN_JSON_LEN(json_) - current_offset_;
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
          value = reinterpret_cast<const int8_t *>(GRN_JSON_VALUE(json_) +
                                                   current_offset_)[0];
        } else if (n_bytes == 2) {
          value = reinterpret_cast<const int16_t *>(GRN_JSON_VALUE(json_) +
                                                    current_offset_)[0];
        } else if (n_bytes == 4) {
          value = reinterpret_cast<const int32_t *>(GRN_JSON_VALUE(json_) +
                                                    current_offset_)[0];
        } else {
          value = reinterpret_cast<const int64_t *>(GRN_JSON_VALUE(json_) +
                                                    current_offset_)[0];
        }
        current_type_ = GRN_JSON_VALUE_INT64;
        GRN_INT64_SET(ctx_, &int64_value_, value);
        current_value_ = &int64_value_;
        current_size_ = 0;
        current_offset_ += size;
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
      auto ctx = ctx_;
      static const char *log_tag = "[json-reader][next][string]";

      if (is_root) {
        size_t size = GRN_JSON_LEN(json_) - current_offset_;
        current_type_ = GRN_JSON_VALUE_STRING;
        GRN_TEXT_SET(ctx_,
                     &string_value_,
                     GRN_JSON_VALUE(json_) + current_offset_,
                     size);
        current_value_ = &string_value_;
        current_size_ = 0;
        current_offset_ += size;
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
      auto ctx = ctx_;
      static const char *log_tag = "[json-reader][next][constant]";

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
