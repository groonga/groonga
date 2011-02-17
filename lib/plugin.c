/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2011 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include "db.h"
#include "plugin_in.h"
#include "ql.h"

static grn_hash *grn_plugins = NULL;

#define PATHLEN(filename) (strlen(filename) + 1)

#ifdef WIN32
#  define grn_dl_open(filename)    LoadLibrary(filename)
#  define grn_dl_open_error_label  "LoadLibrary"
#  define grn_dl_close(dl)         (FreeLibrary(dl) != 0)
#  define grn_dl_close_error_label "FreeLibrary"
#  define grn_dl_sym(dl, symbol)   ((void *)GetProcAddress(dl, symbol))
#  define grn_dl_sym_error_label   "GetProcAddress"
#  define grn_dl_clear_error
#else
#  include <dlfcn.h>
#  define grn_dl_open(filename)    dlopen(filename, RTLD_LAZY | RTLD_LOCAL)
#  define grn_dl_open_error_label  dlerror()
#  define grn_dl_close(dl)         (dlclose(dl) == 0)
#  define grn_dl_close_error_label dlerror()
#  define grn_dl_sym(dl, symbol)   dlsym(dl, symbol)
#  define grn_dl_sym_error_label   dlerror()
#  define grn_dl_clear_error       dlerror()
#endif

grn_id
grn_plugin_get(grn_ctx *ctx, const char *filename)
{
  return grn_hash_get(ctx, grn_plugins, filename, PATHLEN(filename), NULL);
}

const char *
grn_plugin_path(grn_ctx *ctx, grn_id id)
{
  uint32_t key_size;
  return _grn_hash_key(ctx, grn_plugins, id, &key_size);
}

#define GRN_PLUGIN_FUNC_PREFIX "grn_plugin_impl_"

static grn_rc
grn_plugin_call_init (grn_ctx *ctx, grn_id id)
{
  grn_plugin *plugin;
  if (!grn_hash_get_value(ctx, grn_plugins, id, &plugin)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (plugin->init_func) {
    return plugin->init_func(ctx);
  }
  return GRN_SUCCESS;
}

static grn_rc
grn_plugin_call_register(grn_ctx *ctx, grn_id id)
{
  grn_plugin *plugin;
  if (!grn_hash_get_value(ctx, grn_plugins, id, &plugin)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (plugin->register_func) {
    return plugin->register_func(ctx);
  }
  return GRN_SUCCESS;
}

static grn_rc
grn_plugin_call_fin(grn_ctx *ctx, grn_id id)
{
  grn_plugin *plugin;
  if (!grn_hash_get_value(ctx, grn_plugins, id, &plugin)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (plugin->fin_func) {
    return plugin->fin_func(ctx);
  }
  return GRN_SUCCESS;
}

static grn_rc
grn_plugin_initialize(grn_ctx *ctx, grn_plugin *plugin,
                      grn_dl dl, grn_id id, const char *path)
{
  plugin->dl = dl;

#define GET_SYMBOL(type)                                                \
  grn_dl_clear_error;                                                   \
  plugin->type ## _func = grn_dl_sym(dl, GRN_PLUGIN_FUNC_PREFIX #type); \
  if (!plugin->type ## _func) {                                         \
    const char *label;                                                  \
    label = grn_dl_sym_error_label;                                     \
    SERR(label);                                                        \
  }

  GET_SYMBOL(init);
  GET_SYMBOL(register);
  GET_SYMBOL(fin);

#undef GET_SYMBOL

  if (!plugin->init_func || !plugin->register_func || !plugin->fin_func) {
    ERR(GRN_INVALID_FORMAT,
        "init func (%s) %sfound, "
        "register func (%s) %sfound and "
        "fin func (%s) %sfound",
        GRN_PLUGIN_FUNC_PREFIX "init", plugin->init_func ? "" : "not ",
        GRN_PLUGIN_FUNC_PREFIX "register", plugin->register_func ? "" : "not ",
        GRN_PLUGIN_FUNC_PREFIX "fin", plugin->fin_func ? "" : "not ");
  }

  if (!ctx->rc) {
    ctx->impl->plugin_path = path;
    grn_plugin_call_init(ctx, id);
    ctx->impl->plugin_path = NULL;
  }

  return ctx->rc;
}

grn_id
grn_plugin_open(grn_ctx *ctx, const char *filename)
{
  grn_id id;
  grn_dl dl;
  grn_plugin **plugin;

  if ((id = grn_hash_get(ctx, grn_plugins, filename, PATHLEN(filename),
                         (void **)&plugin))) {
    return id;
  }
  if ((dl = grn_dl_open(filename))) {
    if ((id = grn_hash_add(ctx, grn_plugins, filename, PATHLEN(filename),
                           (void **)&plugin, NULL))) {
      *plugin = GRN_GMALLOCN(grn_plugin, 1);
      if (*plugin) {
        if (grn_plugin_initialize(ctx, *plugin, dl, id, filename)) {
          GRN_GFREE(*plugin);
          *plugin = NULL;
        }
      }
      if (!*plugin) {
        grn_hash_delete_by_id(ctx, grn_plugins, id, NULL);
        if (!grn_dl_close(dl)) {
          const char *label;
          label = grn_dl_close_error_label;
          SERR(label);
        }
        id = GRN_ID_NIL;
      }
    } else {
      if (!grn_dl_close(dl)) {
        const char *label;
        label = grn_dl_close_error_label;
        SERR(label);
      }
    }
  } else {
    const char *label;
    label = grn_dl_open_error_label;
    SERR(label);
  }
  return id;
}

grn_rc
grn_plugin_close(grn_ctx *ctx, grn_id id)
{
  grn_plugin *plugin;

  grn_plugin_call_fin(ctx, id);
  if (!grn_hash_get_value(ctx, grn_plugins, id, &plugin)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!grn_dl_close(plugin->dl)) {
    const char *label;
    label = grn_dl_close_error_label;
    SERR(label);
  }
  GRN_GFREE(plugin);
  return grn_hash_delete_by_id(ctx, grn_plugins, id, NULL);
}

void *
grn_plugin_sym(grn_ctx *ctx, grn_id id, const char *symbol)
{
  grn_plugin *plugin;
  grn_dl_symbol func;

  if (!grn_hash_get_value(ctx, grn_plugins, id, &plugin)) {
    return NULL;
  }
  grn_dl_clear_error;
  if (!(func = grn_dl_sym(plugin->dl, symbol))) {
    const char *label;
    label = grn_dl_sym_error_label;
    SERR(label);
  }
  return func;
}

grn_rc
grn_plugins_init(void)
{
  grn_plugins = grn_hash_create(&grn_gctx, NULL, PATH_MAX, sizeof(grn_plugin *),
                                GRN_OBJ_KEY_VAR_SIZE);
  if (!grn_plugins) { return GRN_NO_MEMORY_AVAILABLE; }
  return GRN_SUCCESS;
}

grn_rc
grn_plugins_fin(void)
{
  grn_ctx *ctx = &grn_gctx;
  if (!grn_plugins) { return GRN_INVALID_ARGUMENT; }
  GRN_HASH_EACH(ctx, grn_plugins, id, NULL, NULL, NULL, {
      grn_plugin_close(ctx, id);
    });
  return grn_hash_close(&grn_gctx, grn_plugins);
}

grn_rc
grn_plugin_register_by_path(grn_ctx *ctx, const char *path)
{
  grn_id id;
  grn_obj *db;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return ctx->rc;
  }
  GRN_API_ENTER;
  if (GRN_DB_P(db)) {
    FILE *plugin_file;
    char complemented_path[PATH_MAX], complemented_libs_path[PATH_MAX];

    plugin_file = fopen(path, "r");
    if (plugin_file) {
      fclose(plugin_file);
      id = grn_plugin_open(ctx, path);
    } else {
      strcpy(complemented_path, path);
      strcat(complemented_path, GRN_PLUGIN_SUFFIX);
      plugin_file = fopen(complemented_path, "r");
      if (plugin_file) {
        fclose(plugin_file);
        id = grn_plugin_open(ctx, complemented_path);
        if (id) {
          path = complemented_path;
        }
      } else {
        const char *base_name;

        base_name = strrchr(path, PATH_SEPARATOR[0]);
        if (base_name) {
          complemented_libs_path[0] = '\0';
          strncat(complemented_libs_path, path, base_name - path);
          strcat(complemented_libs_path, PATH_SEPARATOR ".libs");
          strcat(complemented_libs_path, base_name);
          strcat(complemented_libs_path, GRN_PLUGIN_SUFFIX);
          plugin_file = fopen(complemented_libs_path, "r");
          if (plugin_file) {
            fclose(plugin_file);
            id = grn_plugin_open(ctx, complemented_libs_path);
            if (id) {
              path = complemented_libs_path;
            }
          } else {
            ERR(GRN_NO_SUCH_FILE_OR_DIRECTORY,
                "cannot open shared object file: "
                "No such file or directory: <%s> and <%s>",
                complemented_path, complemented_libs_path);
          }
        }
      }
    }

    if (id) {
      ctx->impl->plugin_path = path;
      ctx->rc = grn_plugin_call_register(ctx, id);
      ctx->impl->plugin_path = NULL;
      if (ctx->rc) {
        grn_plugin_close(ctx, id);
      }
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid db assigned");
  }
  GRN_API_RETURN(ctx->rc);
}

#ifdef WIN32
static char *win32_plugins_dir = NULL;
static const char *
default_plugins_dir(void)
{
  if (!win32_plugins_dir) {
    wchar_t dll_filename[MAX_PATH];
    DWORD dll_filename_size;
    dll_filename_size = GetModuleFileNameW(NULL, dll_filename, MAX_PATH);
    if (dll_filename_size == 0) {
      win32_plugins_dir = GRN_PLUGINS_DIR;
    } else {
      char *plugins_dir;
      DWORD ansi_dll_filename_size;
      ansi_dll_filename_size =
        WideCharToMultiByte(CP_ACP, 0, dll_filename, dll_filename_size,
                            NULL, 0, NULL, NULL);
      if (ansi_dll_filename_size == 0) {
        win32_plugins_dir = GRN_PLUGINS_DIR;
      } else {
        char *path;
        const char *relative_path = GRN_RELATIVE_PLUGINS_DIR;
        plugins_dir = malloc(ansi_dll_filename_size +
                             strlen(relative_path));
        WideCharToMultiByte(CP_ACP, 0, dll_filename, dll_filename_size,
                            plugins_dir, ansi_dll_filename_size,
                            NULL, NULL);
        if ((path = strrchr(plugins_dir, '\\'))) {
          *path = '\0';
        }
        path = strrchr(plugins_dir, '\\');
        if (path && (strcasecmp(path + 1, "bin") == 0 ||
                     strcasecmp(path + 1, "lib") == 0)) {
          *path = '\0';
        } else {
          path = plugins_dir + strlen(plugins_dir);
        }
        *path = '\\';
        path++;
        while (*relative_path) {
          if (*relative_path == '/') {
            *path = '\\';
          } else {
            *path = *relative_path;
          }
          relative_path++;
          path++;
        }
        *path = '\0';
        win32_plugins_dir = plugins_dir;
      }
    }
  }
  return win32_plugins_dir;
}

static void
normalize_path_separator(char *path)
{
  while (*path) {
#ifdef __GNUC__
    if (*path == '\\') {
      *path = '/';
    }
#else  /* __GNUC__ */
    if (*path == '/') {
      *path = '\\';
    }
#endif /* __GNUC__ */
    path++;
  }
}
#else /* WIN32 */
#  define default_plugins_dir() GRN_PLUGINS_DIR;
#  define normalize_path_separator(path) path;
#endif /* WIN32 */

grn_rc
grn_plugin_register(grn_ctx *ctx, const char *name)
{
  const char *plugins_dir;
  char dir_last_char;
  char path[PATH_MAX];

  plugins_dir = getenv("GRN_PLUGINS_DIR");
  if (!plugins_dir) {
    plugins_dir = default_plugins_dir();
  }
  strcpy(path, plugins_dir);
  dir_last_char = plugins_dir[strlen(plugins_dir) - 1];
  if (dir_last_char != PATH_SEPARATOR[0]) {
    strcat(path, PATH_SEPARATOR);
  }
  strcat(path, name);
  normalize_path_separator(path);
  return grn_plugin_register_by_path(ctx, path);
}
