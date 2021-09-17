#!/bin/sh

source_dir=$(cd $(dirname $0) && pwd)

list_paths()
{
    variable_name=$1
    echo "$variable_name = \\"
    sort | \
    sed \
      -e 's,^,	,' \
      -e 's,$, \\,'
    echo "	\$(NULL)"
    echo
}

# Sphinx related
## absolute source file path list.
(cd $source_dir &&
   find "source" -type f -not -name '*.pyc' | \
      sed -e 's,^,$(top_srcdir)/doc/,g' | \
      list_paths "absolute_source_files")

## absolute theme file path list.
(cd $source_dir &&
   find "themes" -type f | \
       sed -e 's,^,$(top_srcdir)/doc/,g' | \
       list_paths "absolute_theme_files")

## source file path list from doc/.
(cd $source_dir &&
   find "source" -type f -not -name '*.pyc' | \
     list_paths "source_files_relative_from_doc_dir")

## theme file path list from doc/.
(cd $source_dir &&
   find "themes" -type f | \
       list_paths "theme_files_relative_from_doc_dir")

# gettext related
## po file base paths
(cd locale/en/LC_MESSAGES; find . -type f -name '*.pot') | \
    sed \
      -e 's,^\./,,' \
      -e 's,pot$,po,' | \
    list_paths "po_files"

## po file paths relative from locale/$LANG/ dir.
(cd locale/en/LC_MESSAGES; find . -type f -name '*.pot') | \
    sed \
      -e 's,^\.,LC_MESSAGES,' \
      -e 's,pot$,po,' | \
    list_paths "po_files_relative_from_locale_dir"

## edit file base paths
(cd locale/en/LC_MESSAGES; find . -type f -name '*.pot') | \
    sed \
      -e 's,^\./,,' \
      -e 's,pot$,edit,' | \
    list_paths "edit_po_files"

## edit file paths relative from locale/$LANG/ dir.
(cd locale/en/LC_MESSAGES; find . -type f -name '*.pot') | \
    sed \
      -e 's,^\.,LC_MESSAGES,' \
      -e 's,pot$,edit,' | \
    list_paths "edit_po_files_relative_from_locale_dir"

## mo file paths relative from locale/$LANG/ dir.
(cd locale/en/LC_MESSAGES; find . -type f -name '*.pot') | \
    sed \
      -e 's,^\./,,' \
      -e 's,pot$,mo,' | \
    list_paths "mo_files"

## mo file paths relative from locale/$LANG/ dir.
(cd locale/en/LC_MESSAGES; find . -type f -name '*.pot') | \
    sed \
      -e 's,^\.,LC_MESSAGES,' \
      -e 's,pot$,mo,' | \
    list_paths "mo_files_relative_from_locale_dir"

# output files
## HTML file path list relative from locale/$LANG/ dir.
(cd locale/en; find html -type f) | \
    list_paths "html_files_relative_from_locale_dir"
