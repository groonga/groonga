/* -*- c-file-style: "gnu" -*- */

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

void test_index_create(void);

static gchar *tmp_directory;
static gchar *index_path;
static sen_index *groonga_index;

void
startup(void)
{
  tmp_directory = g_build_filename(g_get_tmp_dir(), "test-index", NULL);
}

void
shutdown(void)
{
  g_free(tmp_directory);
}

void
setup(void)
{
  g_remove(tmp_directory);
  g_mkdir_with_parents(tmp_directory, 0700);
  groonga_index = NULL;
  index_path = g_build_filename(tmp_directory, "index", NULL);
}

static void
remove_tmp_directory(void)
{
  GDir *dir;
  const gchar *name;
  dir = g_dir_open(tmp_directory, 0, NULL);
  if (!dir) {
    return;
  }
  while ((name = g_dir_read_name(dir))) {
    gchar *filename;
    filename = g_build_filename(tmp_directory, name, NULL);
    g_remove(filename);
  }
  g_dir_close(dir);
  g_remove(tmp_directory);
}

void
teardown(void)
{
  if (groonga_index) {
    sen_index_close(groonga_index);
  }
  if (index_path) {
    sen_index_remove(index_path);
    g_free(index_path);
    index_path = NULL;
  }
  remove_tmp_directory();
}

static const gchar *
strconcat(const char *string1, const char *string2)
{
  return cut_take_string(g_strconcat(string1, string2, NULL));
}

void
test_index_create(void)
{
  groonga_index = sen_index_create(index_path, 0, 0, 0, sen_enc_default);
  cut_assert(groonga_index);
  cut_assert_file_exist(strconcat(index_path, ".SEN"));
  cut_assert_file_exist(strconcat(index_path, ".SEN.i"));
  cut_assert_file_exist(strconcat(index_path, ".SEN.i.c"));
  cut_assert_file_exist(strconcat(index_path, ".SEN.l"));
}
