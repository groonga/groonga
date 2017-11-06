#!/usr/bin/env ruby

base_dir = ARGV[0]

@targets = {}
def register(character_code, description)
  @targets[character_code] = description
end

property_aliases = {}
File.open("#{base_dir}/PropertyValueAliases.txt") do |file|
  file.each_line do |line|
    case line
    when /\A[a-z]/i
      target, abbrev, name, = line.chomp.split(/\s*;\s*/)
      next if abbrev == "n/a"
      property_aliases[abbrev] = name
    end
  end
end

File.open("#{base_dir}/PropList.txt") do |file|
  file.each_line do |line|
    case line.chomp
    when /\A([\da-f]{4,5})(?:\.\.([\da-f]{4,5})) +; .+? \# (.{2})/i
      start = $1
      last = $2
      property_value_alias = $3
      property_value = property_aliases[property_value_alias]
      property_value ||= property_value_alias
      case property_value
      when "Dash_Punctuation",
        "Open_Punctuation",
        "Close_Punctuation",
        "Connector_Punctuation",
        "Other_Punctuation",
        "Math_Symbol",
        "Currency_Symbol",
        "Modifier_Symbol",
        "Other_Symbol"
        if last.nil?
          register(start.to_i(16), property_value)
        else
          (start.to_i(16)..last.to_i(16)).each do |character_code|
            register(character_code, property_value)
          end
        end
      end
    end
  end
end

File.open("#{base_dir}/Blocks.txt") do |file|
  file.each_line do |line|
    case line.chomp
    when /\A([\da-f]{4,5})\.\.([\da-f]{4,5}); (.+)\z/i
      start = $1
      last = $2
      description = $3
      case description
      when "CJK Symbols and Punctuation",
        "Enclosed CJK Letters and Months",
        "CJK Compatibility",
        "CJK Compatibility Forms"
        (start.to_i(16)..last.to_i(16)).each do |character_code|
          register(character_code, description)
        end
      end
    end
  end
end

@targets.keys.sort.each do |character_code|
  description = @targets[character_code]
  character = [character_code].pack("U")
  puts("%#x: %s: %s" % [character_code, character, description])
end
