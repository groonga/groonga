#!/usr/bin/env ruby
#
# Copyright (C) 2024  Kouhei Sutou <kou@clear-code.com>
# Copyright (C) 2024  Kodama Takuya <otegami@clear-code.com>
# Copyright (C) 2024  Abe Tomoaki <abe@clear-code.com>
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

def have_diactritical_combining_character?(character)
  code_points = character.unicode_normalize(:nfd).codepoints
  code_points.any? do |code_point|
    (0x0300..0x036f).cover?(code_point)
  end
end

def base_character(character)
  character.unicode_normalize(:nfd).chars.first
end

def base_character_is_lower_alphabet?(character)
  base_code_point = base_character(character).codepoints.first
  (("a".codepoints.first)..("z".codepoints.first)).cover?(base_code_point)
end

def base_character_is_capital_alphabet?(character)
  base_code_point = base_character(character).codepoints.first
  (("A".codepoints.first)..("Z".codepoints.first)).cover?(base_code_point)
end

def base_character_is_alphabet?(character)
  base_character_is_lower_alphabet?(character) || base_character_is_capital_alphabet?(character)
end

target_characters = {
  downcase: ARGV[0].downcase,
  upcase: ARGV[0].upcase
}

puts '## Generate mapping about Unicode and UTF-8'
(0x0000..0xffff).each do |code_point|
  begin
    character = code_point.chr("UTF-8")
  rescue RangeError
    next
  end
  next unless have_diactritical_combining_character?(character)
  next unless base_character_is_lower_alphabet?(character)
  next unless base_character(character) == target_characters[:downcase]
  pp ["U+%04x" % code_point, character, character.bytes.collect {|b| "%#02x" % b}]
end

puts "-" * 50

puts '## Generate target characters'
(0x0000..0xffff).each do |code_point|
  begin
    character = code_point.chr("UTF-8")
  rescue RangeError
    next
  end
  next unless have_diactritical_combining_character?(character)
  next unless base_character_is_alphabet?(character)
  next unless target_characters.values.any?(base_character(character))
  print character
end
