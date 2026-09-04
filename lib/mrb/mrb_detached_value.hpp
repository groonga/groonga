/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

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

#pragma once

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/string.h>

#include <string>
#include <vector>

namespace grn {
  namespace mrb {
    /* A value that can be moved between mrb_states and threads.
     *
     * Each grn_ctx has its own mrb_state and mruby isn't thread
     * safe. A value in a mrb_state can't be used from another
     * mrb_state. So a value is converted to this representation that
     * doesn't depend on any mrb_state.
     *
     * Only nil, true, false, Integer, Float, Symbol, String, Array
     * and Hash can be converted. */
    class DetachedValue {
    public:
      enum class Type {
        NIL,
        BOOLEAN,
        INTEGER,
        FLOAT,
        SYMBOL,
        STRING,
        ARRAY,
        HASH,
      };

      static constexpr int max_depth = 32;

      DetachedValue()
        : type_(Type::NIL),
          boolean_(false),
          integer_(0),
          float_(0.0),
          string_(),
          array_(),
          hash_keys_(),
          hash_values_()
      {
      }

      /* Convert a value in `mrb` to a detached value. This doesn't
       * raise. `false` is returned with the class name of the value
       * that can't be converted in `error_class_name` on failure. */
      static bool
      from_mrb(mrb_state *mrb,
               mrb_value value,
               DetachedValue &output,
               const char **error_class_name)
      {
        return from_mrb(mrb, value, output, error_class_name, 0);
      }

      /* Convert this to a value in `mrb`. The returned value is
       * protected by the arena of `mrb`. */
      mrb_value
      to_mrb(mrb_state *mrb) const
      {
        switch (type_) {
        case Type::NIL:
          return mrb_nil_value();
        case Type::BOOLEAN:
          return mrb_bool_value(boolean_);
        case Type::INTEGER:
          return mrb_int_value(mrb, integer_);
        case Type::FLOAT:
          return mrb_float_value(mrb, float_);
        case Type::SYMBOL:
          return mrb_symbol_value(
            mrb_intern(mrb, string_.data(), string_.size()));
        case Type::STRING:
          return mrb_str_new(mrb, string_.data(), string_.size());
        case Type::ARRAY:
          {
            auto array =
              mrb_ary_new_capa(mrb, static_cast<mrb_int>(array_.size()));
            for (const auto &element : array_) {
              auto arena_index = mrb_gc_arena_save(mrb);
              mrb_ary_push(mrb, array, element.to_mrb(mrb));
              mrb_gc_arena_restore(mrb, arena_index);
            }
            return array;
          }
        case Type::HASH:
          {
            auto hash =
              mrb_hash_new_capa(mrb, static_cast<mrb_int>(hash_keys_.size()));
            for (size_t i = 0; i < hash_keys_.size(); ++i) {
              auto arena_index = mrb_gc_arena_save(mrb);
              mrb_hash_set(mrb,
                           hash,
                           hash_keys_[i].to_mrb(mrb),
                           hash_values_[i].to_mrb(mrb));
              mrb_gc_arena_restore(mrb, arena_index);
            }
            return hash;
          }
        default:
          return mrb_nil_value();
        }
      }

    private:
      static bool
      from_mrb(mrb_state *mrb,
               mrb_value value,
               DetachedValue &output,
               const char **error_class_name,
               int depth)
      {
        if (depth > max_depth) {
          *error_class_name = "too deep value";
          return false;
        }
        switch (mrb_type(value)) {
        case MRB_TT_FALSE:
          if (mrb_nil_p(value)) {
            output.type_ = Type::NIL;
          } else {
            output.type_ = Type::BOOLEAN;
            output.boolean_ = false;
          }
          return true;
        case MRB_TT_TRUE:
          output.type_ = Type::BOOLEAN;
          output.boolean_ = true;
          return true;
        case MRB_TT_INTEGER:
          output.type_ = Type::INTEGER;
          output.integer_ = mrb_integer(value);
          return true;
        case MRB_TT_FLOAT:
          output.type_ = Type::FLOAT;
          output.float_ = mrb_float(value);
          return true;
        case MRB_TT_SYMBOL:
          {
            mrb_int length;
            auto name = mrb_sym_name_len(mrb, mrb_symbol(value), &length);
            output.type_ = Type::SYMBOL;
            output.string_.assign(name, length);
            return true;
          }
        case MRB_TT_STRING:
          output.type_ = Type::STRING;
          output.string_.assign(RSTRING_PTR(value), RSTRING_LEN(value));
          return true;
        case MRB_TT_ARRAY:
          {
            output.type_ = Type::ARRAY;
            auto length = RARRAY_LEN(value);
            output.array_.resize(length);
            for (mrb_int i = 0; i < length; ++i) {
              if (!from_mrb(mrb,
                            mrb_ary_entry(value, i),
                            output.array_[i],
                            error_class_name,
                            depth + 1)) {
                return false;
              }
            }
            return true;
          }
        case MRB_TT_HASH:
          {
            output.type_ = Type::HASH;
            auto arena_index = mrb_gc_arena_save(mrb);
            auto keys = mrb_hash_keys(mrb, value);
            auto length = RARRAY_LEN(keys);
            output.hash_keys_.resize(length);
            output.hash_values_.resize(length);
            for (mrb_int i = 0; i < length; ++i) {
              auto key = mrb_ary_entry(keys, i);
              if (!from_mrb(mrb,
                            key,
                            output.hash_keys_[i],
                            error_class_name,
                            depth + 1)) {
                mrb_gc_arena_restore(mrb, arena_index);
                return false;
              }
              if (!from_mrb(mrb,
                            mrb_hash_get(mrb, value, key),
                            output.hash_values_[i],
                            error_class_name,
                            depth + 1)) {
                mrb_gc_arena_restore(mrb, arena_index);
                return false;
              }
            }
            mrb_gc_arena_restore(mrb, arena_index);
            return true;
          }
        default:
          *error_class_name = mrb_obj_classname(mrb, value);
          return false;
        }
      }

      Type type_;
      bool boolean_;
      mrb_int integer_;
      mrb_float float_;
      /* Symbol name or String content */
      std::string string_;
      std::vector<DetachedValue> array_;
      std::vector<DetachedValue> hash_keys_;
      std::vector<DetachedValue> hash_values_;
    };
  } // namespace mrb
} // namespace grn
