#!/usr/bin/env ruby
#
# Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

require "json"

entities = JSON.parse(File.read(ARGV[0] || "entities.json"))

puts(<<-HEADER)
/*
  Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
  Don't edit this file by hand.
  This is generated automatically by normalizer_html_expand_named_char_ref.rb.
*/

#include "grn_str.h"
#include <string.h>

HEADER

puts(<<-FUNCTION_START)
/* Mapping table:
   https://html.spec.whatwg.org/multipage/named-characters.html#named-character-references */
static bool
html_expand_named_char_ref(grn_ctx *ctx,
                           grn_obj *buffer,
                           const char *name,
                           size_t length)
{
FUNCTION_START

target_entities = {}
entities.each do |name, entity|
  normalized_name = name.gsub(/\A&|;\z/, "")
  target_entities[normalized_name] = entity
end
length_grouped_target_entities = target_entities.group_by do |name, _|
  name.length
end
puts("  switch (length) {")
length_grouped_target_entities.keys.sort.each do |length|
  puts("  case #{length} :")
  length_target_entities = length_grouped_target_entities[length]
  first_char_grouped_target_entities = length_target_entities.group_by do |name, _|
    name[0]
  end
  puts("    switch (name[0]) {")
  first_char_grouped_target_entities.keys.sort.each do |first_char|
    puts("    case '#{first_char}' :")
    first_char_grouped_target_entities[first_char].each do |name, entity|
      puts("      if (strncmp(name + 1, \"#{name[1..-1]}\", length - 1) == 0) {")
      entity["codepoints"].each do |code_point|
        puts("        grn_text_code_point(ctx, buffer, #{code_point});")
      end
      puts("        return true;")
      puts("      }")
    end
    puts("      break;")
  end
  puts("    default :")
  puts("      break;")
  puts("    }")
  puts("    break;")
end
puts("  default :")
puts("    break;")
puts("  }")

puts(<<-FUNCTION_END)
  return false;
}
FUNCTION_END
