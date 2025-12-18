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
#  include <string>

#  include <simdjson.h>
#endif


namespace {
  enum class Type {
    kObject   = 0,
    kArray    = 1,
    kString   = 2,
    kInteger  = 3,
    kDouble   = 4,
    kConstant = 5,
  };

  namespace metadata {
    enum class Constant {
      kTrue = 0,
      kFalse = 1,
      kNull = 2,
    };
  }

  struct Tag {
    Type type;
    bool is_embedded;
    uint8_t metadata;
    uint32_t data;
  };

#ifdef GRN_WITH_SIMDJSON
  uint32_t
  pack_tag(Type type, bool is_embedded, uint8_t metadata, uint32_t data) {
    uint32_t tag = static_cast<uint32_t>(type);
    if (is_embedded) {
      tag |= (1 << 3);
    }
    tag |= (metadata << 4);
    tag |= (data << 8);
    return tag;
  }

  struct BaseTagWriter {
    virtual ~BaseTagWriter() = default;
    virtual void write(Type type, bool is_embedded, uint8_t metadata, uint32_t data) = 0;
  };

  struct TagWriter : public BaseTagWriter {
    grn_ctx *ctx_;
    grn_obj buffer;
    uint32_t size16;
    uint32_t size32;

    TagWriter(grn_ctx *ctx) : ctx_(ctx), buffer(), size16(0), size32(0) {
      GRN_BINARY_INIT(&buffer, 0);
    }

    ~TagWriter() {
      GRN_OBJ_FIN(ctx_, &buffer);
    }

    void write(Type type, bool is_embedded, uint8_t metadata, uint32_t data) override {
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

  struct RootTagWriter : public BaseTagWriter  {
    grn_ctx *ctx_;
    grn_obj buffer;

    RootTagWriter(grn_ctx *ctx) : ctx_(ctx), buffer() {
      GRN_BINARY_INIT(&buffer, 0);
    }

    ~RootTagWriter() {
      GRN_OBJ_FIN(ctx_, &buffer);
    }

    void write(Type type, bool is_embedded, uint8_t metadata, uint32_t data) override {
      assert(data == 0); // Root tag's data must be zero
      auto tag = pack_tag(type, is_embedded, metadata, 0);
      GRN_UINT8_PUT(ctx_, &buffer, tag);
    }
  };

  class JSONParser {
  public:
    JSONParser(grn_ctx *ctx,
               grn_obj *input,
               grn_obj *output) : ctx_(ctx), input_(input), output_(output), root_tag_writer_(ctx_) {
    }

    ~JSONParser() = default;

    void parse() {
      using json_type = simdjson::fallback::ondemand::json_type;
      const char *tag = "[json-parser][parse]";
      auto ctx = ctx_;

      auto rc = grn_bulk_reserve(ctx_, input_, simdjson::SIMDJSON_PADDING);
      if (rc != GRN_SUCCESS) {
        return;
      }

      simdjson::ondemand::parser parser;
      auto document = parser.iterate(GRN_TEXT_VALUE(input_),
                                     GRN_TEXT_LEN(input_),
                                     GRN_BULK_WSIZE(input_));
      if (document.error() != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to parse: %s",
            tag, simdjson::error_message(document.error()));
        return;
      }

      json_type type;
      auto error_code = document.type().get(type);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get root value type: %s", tag, simdjson::error_message(error_code));
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
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s number isn't supported yet", tag);
        return;
      case json_type::string:
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "%s string isn't supported yet", tag);
        return;
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
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get root value: %s", tag, simdjson::error_message(document.error()));
        return;
      }

      grn_bulk_write(ctx_, output_,
                     GRN_BULK_HEAD(&(root_tag_writer_.buffer)),
                     GRN_BULK_VSIZE(&(root_tag_writer_.buffer)));
    }

  private:
    grn_ctx *ctx_;
    grn_obj *input_;
    grn_obj *output_;
    RootTagWriter root_tag_writer_;

    template <typename TagWriter, typename Container>
    bool parse_boolean(TagWriter& tag_writer, Container& container)
    {
      const char *tag = "[json-parser][parse][boolean]";
      auto ctx = ctx_;

      bool value;
      auto error_code = container.get_bool().get(value);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get value: %s", tag, simdjson::error_message(error_code));
        return false;
      }
      uint8_t metadata = value ?
        static_cast<uint8_t>(metadata::Constant::kTrue) :
        static_cast<uint8_t>(metadata::Constant::kFalse);
      tag_writer.write(Type::kConstant, true, metadata, 0);
      return true;
    }

    template <typename TagWriter, typename Container>
    bool parse_null(TagWriter& tag_writer, Container& container)
    {
      const char *tag = "[json-parser][parse][null]";
      auto ctx = ctx_;

      bool is_null;
      auto error_code = container.is_null().get(is_null);
      if (error_code != simdjson::SUCCESS) {
        ERR(GRN_INVALID_ARGUMENT,
            "%s failed to get value: %s", tag, simdjson::error_message(error_code));
        return false;
      }
      uint8_t metadata = static_cast<uint8_t>(metadata::Constant::kNull);
      tag_writer.write(Type::kConstant, true, metadata, 0);
      return true;
    }
  };
#endif

  class JSONReader {
  public:
    JSONReader(grn_ctx *ctx,
               grn_obj *json) : ctx_(ctx), json_(json), current_offset_(0), current_type_(GRN_JSON_VALUE_UNKNOWN), current_value_(nullptr), current_size_(0) {
      GRN_VOID_INIT(&null_value_);
      GRN_BOOL_INIT(&bool_value_, 0);
      GRN_INT64_INIT(&int64_value_, 0);
      GRN_FLOAT_INIT(&float_value_, 0);
    }

    ~JSONReader() {
      GRN_OBJ_FIN(ctx_, &null_value_);
      GRN_OBJ_FIN(ctx_, &bool_value_);
      GRN_OBJ_FIN(ctx_, &int64_value_);
      GRN_OBJ_FIN(ctx_, &float_value_);
    };

    grn_rc next() {
      static const char *tag = "[json-reader][next]";
      auto ctx = ctx_;

      if (current_offset_ >= GRN_JSON_LEN(json_)) {
        return GRN_END_OF_DATA;
      }
      if (current_offset_ == 0) {
        auto root_tag = unpack_tag(GRN_JSON_VALUE(json_)[current_offset_]);
        if (root_tag.type == Type::kConstant) {
          current_offset_ += sizeof(uint8_t);
          current_size_ = 0;
          auto constant_metadata = static_cast<metadata::Constant>(root_tag.metadata);
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
                "%s invalid constant metadata: %u", tag, root_tag.metadata);
            return ctx->rc;
          }
        } else {
          ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
              "%s unsupported root value type", tag);
          return ctx->rc;
        }
      } else {
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
            "%s root value is only supported", tag);
        return ctx->rc;
      }
      return ctx->rc;
    }

    grn_json_value_type type() {
      return current_type_;
    }

    grn_obj *value() {
      return current_value_;
    }

    size_t size() {
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

    Tag unpack_tag(uint32_t raw_tag) {
      Tag tag;
      tag.type = static_cast<Type>(raw_tag & 0b111);
      tag.is_embedded = (raw_tag & (0b10000));
      tag.metadata = ((raw_tag >> 4) & (0b00001111));
      tag.data = (raw_tag >> 8);
      return tag;
    }
  };

  class JSONStringifier {
  public:
    JSONStringifier(grn_ctx *ctx, grn_obj *json, grn_obj *buffer) : ctx_(ctx), json_(json), buffer_(buffer) {
    }
    ~JSONStringifier() = default;

    void stringify() {
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

    bool stringify_value(JSONReader &reader) {
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
}

extern "C" {
struct _grn_json_parser {
#ifdef GRN_WITH_SIMDJSON
  JSONParser *parser;
#else
  void *parser;
#endif
  };

grn_json_parser *
grn_json_parser_open(grn_ctx *ctx,
                     grn_obj *input,
                     grn_obj *output)
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
grn_json_reader_open(grn_ctx *ctx,
                     grn_obj *json)
{
  GRN_API_ENTER;
  auto reader = static_cast<grn_json_reader *>(GRN_MALLOC(sizeof(grn_json_reader)));
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
