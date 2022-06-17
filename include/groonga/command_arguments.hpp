/*
  Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>

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

#include <groonga.h>
#include <groonga/plugin.h>

#include <string>

namespace grn {
  class CommandArguments {
  public:
    CommandArguments(grn_ctx *ctx,
                     grn_user_data *user_data)
      : ctx_(ctx),
        user_data_(user_data)
    {
    }

    ~CommandArguments()
    {
    }

    grn_obj *
    get(const char *prefix,
        const char *name,
        const char *fallback_name)
    {
      std::string name_buffer;
      const char *full_name;
      if (prefix) {
        name_buffer = prefix;
        name_buffer += name;
        full_name = name_buffer.c_str();
      } else {
        full_name = name;
      }
      auto arg = grn_plugin_proc_get_var(ctx_, user_data_, full_name, -1);
      if (arg && GRN_TEXT_LEN(arg) > 0) {
        return arg;
      }

      if (fallback_name) {
        const char *full_fallback_name;
        if (prefix) {
          name_buffer = prefix;
          name_buffer += fallback_name;
          full_fallback_name = name_buffer.c_str();
        } else {
          full_fallback_name = fallback_name;
        }
        arg = grn_plugin_proc_get_var(ctx_, user_data_, full_fallback_name, -1);
        if (arg && GRN_TEXT_LEN(arg) > 0) {
          return arg;
        }
      }

      return arg;
    }

    grn_obj *
    get(const char *name)
    {
      return get(nullptr, name, nullptr);
    }

    grn_obj *
    get(const char *prefix,
        const char *name)
    {
      return get(prefix, name, nullptr);
    }

    grn_obj *
    get(const char *prefix,
        const char *fallback_prefix,
        const char *name,
        const char *fallback_name)
    {
      auto arg = get(prefix, name, fallback_name);
      if (arg && GRN_TEXT_LEN(arg) > 0) {
        return arg;
      }
      return get(fallback_prefix, name, fallback_name);
    }

    grn_raw_string
    get_string(const char *name)
    {
      return arg_to_string(get(name));
    }

    grn_raw_string
    get_string(const char *prefix,
               const char *name)
    {
      return arg_to_string(get(prefix, name));
    }

    grn_raw_string
    get_string(const char *prefix,
               const char *name,
               const char *fallback_name)
    {
      return arg_to_string(get(prefix, name, fallback_name));
    }

    grn_raw_string
    get_string(const char *prefix,
               const char *fallback_prefix,
               const char *name,
               const char *fallback_name)
    {
      return arg_to_string(get(prefix, fallback_prefix, name, fallback_name));
    }

    int32_t
    get_int32(const char *name,
              int32_t default_value)
    {
      return arg_to_int32(get(name),
                          default_value);
    }

    int32_t
    get_int32(const char *prefix,
              const char *name,
              int32_t default_value)
    {
      return arg_to_int32(get(prefix, name),
                          default_value);
    }

    int32_t
    get_int32(const char *prefix,
              const char *name,
              const char *fallback_name,
              int32_t default_value)
    {
      return arg_to_int32(get(prefix, name, fallback_name),
                          default_value);
    }

    int32_t
    get_int32(const char *prefix,
              const char *fallback_prefix,
              const char *name,
              const char *fallback_name,
              int32_t default_value)
    {
      return arg_to_int32(get(prefix, fallback_prefix, name, fallback_name),
                          default_value);
    }

  private:
    grn_ctx *ctx_;
    grn_user_data *user_data_;

    grn_raw_string
    arg_to_string(grn_obj *arg)
    {
      grn_raw_string string = {nullptr, 0};
      GRN_RAW_STRING_FILL(string, arg);
      return string;
    }

    int32_t
    arg_to_int32(grn_obj *arg,
                 int32_t default_value);
  };
}
