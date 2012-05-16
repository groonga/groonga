#!/usr/bin/env ruby
# -*- coding: utf-8 -*-

require 'fileutils'
require 'optparse'

TABLE_CREATE = "table_create Geo TABLE_HASH_KEY ShortText\n"
COLUMN_CREATE = "column_create Geo distance COLUMN_SCALAR Int32\n"
LOAD = <<-EOF
load --table Geo
[
{"_key": "the record for geo_distance() result"}
]
EOF
CREATE_RESULT = "[[0,0.0,0.0],true]\n"
LOAD_RESULT = "[[0,0.0,0.0],1]\n"

SELECT = "select Geo --output_columns distance "

SELECT_PRE = "[[0,0.0,0.0],[[[1],[[\"_score\",\"Int32\"]],["
SELECT_POST = "]]]]"

class GrnTestData

  attr :csv_file
  attr :options

  attr_accessor :longitude_start_degree
  attr_accessor :latitude_start_degree
  attr_accessor :longitude_end_degree
  attr_accessor :latitude_end_degree

  attr_accessor :longitude_start
  attr_accessor :latitude_start
  attr_accessor :longitude_end
  attr_accessor :latitude_end

  attr_accessor :distance
  attr_accessor :output_filename

  def initialize(options = {})
    @csv_file = options[:csv]
    @options = options
  end

  def parse_line_data(data)
    @longitude_start_degree, @latitude_start_degree,
      @longitude_end_degree, @latitude_end_degree,
      @longitude_start, @latitude_start,
      @longitude_end, @latitude_end,
      @distance, @output_filename = data.chomp.split(",")

    @longitude_start_degree = @longitude_start_degree.to_i
    @latitude_start_degree = @latitude_start_degree.to_i
    @longitude_end_degree = @longitude_end_degree.to_i
    @latitude_end_degree = @latitude_end_degree.to_i
  end

end

def long?(start_lng_deg, end_lng_deg)
  if start_lng_deg != end_lng_deg and
      ((start_lng_deg > 0 && end_lng_deg.to_i < 0) or
      (start_lng_deg < 0 && end_lng_deg.to_i > 0)) and
      start_lng_deg.abs + end_lng_deg.to_i.abs > 180
    # the difference in longitude striding accross meridian is over
    # 180 degree.
    true
  else
    false
  end
end

def get_quadrant(lng, lat)
  if lng > 0 and lat > 0
    "1st"
  elsif lng < 0 and lat > 0
    "2nd"
  elsif lng < 0 and lat < 0
    "3rd"
  elsif lng > 0 and lat < 0
    "4th"
  else
    nil
  end
end

def meridian?(lng, lat)
end

def equator?(lng, lat)
end

def east_axis?(lng, lat)
  lng >= 0 and lat == 0
end

def west_axis?(lng, lat)
  lng <= 0 and lat == 0
end

def north_axis?(lng, lat)
  lng == 0 and lat >= 0
end

def south_axis?(lng, lat)
  lng == 0 and lat <= 0
end

def point?(start_lng, start_lat, end_lng, end_lat)
  start_lng == end_lng and start_lat == end_lat
end

def get_quadrant_to(start_lng, start_lat, end_lng, end_lat)
  ret = ""
  squadrant = get_quadrant(start_lng, start_lat)
  equadrant = get_quadrant(end_lng, end_lat)
  # p squadrant
  # p equadrant
  # p start_lng
  # p start_lat
  # p end_lng
  # p end_lat
  if (start_lng == end_lng and start_lng == 0)
    ret = "meridian"
  elsif (start_lat == end_lat and start_lat == 0)
    ret = "equator"
  elsif !squadrant or !equadrant
    if (not squadrant) and (not equadrant)
      if east_axis?(start_lng, start_lat) and north_axis?(end_lng, end_lat) or
          north_axis?(start_lng, start_lat) and east_axis?(end_lng, end_lat)
        return "1st"
      elsif north_axis?(start_lng, start_lat) and west_axis?(end_lng, end_lat) or
          west_axis?(start_lng, start_lat) and north_axis?(end_lng, end_lat)
        return "2nd"
      elsif west_axis?(start_lng, start_lat) and south_axis?(end_lng, end_lat) or
          south_axis?(start_lng, start_lat) and west_axis?(end_lng, end_lat)
        return "3rd"
      elsif east_axis?(start_lng, start_lat) and south_axis?(end_lng, end_lat) or
          south_axis?(start_lng, start_lat) and east_axis?(end_lng, end_lat)
        return "4th"
      end
    elsif not squadrant
      ret = equadrant
    elsif not equadrant
      ret = squadrant
    end
  else
    if squadrant == equadrant
      ret = equadrant
    else
      ret = "#{squadrant}to#{equadrant}"
    end
  end
  ret
end

def get_point(lng, lat)
  ret = ""
  # lng -> lat
  lng_desc = {
    0 => {
      90 => "north_pole_on_meridian",
      89 => "near_north_pole_on_meridian",
      1 => "near_positive_origin_on_meridian",
      0 => "origin_on_meridian",
      -1 => "near_negative_origin_on_meridian",
      -89 => "near_south_pole_on_meridian",
      -90 => "south_pole_on_meridian",
    },
    -180 => "west_edge",
    -179 => "near_west_edge",
    -91 => "west_near_ninety_degrees",
    -90 => "west_ninety_degrees",
    -89 => "west_near_ninety_degrees",
    -1 => "near_negative_origin",
    1 => "near_positive_origin",
    89 => "east_near_ninety_degrees",
    90 => "east_ninety_degrees",
    91 => "east_near_ninety_degrees",
    179 => "near_east_edge",
  }
  lat_desc = {
    0 => {
      -180 => "west_edge_on_equator",
      -179 => "near_west_edge_on_equator",
      -91 => "west_near_ninety_degrees_on_equator",
      -90 => "west_ninety_degrees_on_equator",
      -89 => "west_near_ninety_degrees_on_equator",
      -1 => "near_nagative_origin_on_equator",
      0 => "origin",
      1 => "near_positive_origin_on_equator",
      89 => "east_near_ninetydegrees_on_equator",
      90 => "east_ninety_degrees_on_equator",
      91 => "east_near_ninety_degrees_on_equator",
      179 => "near_east_edge_on_equator",
    },
    90 => "north_pole",
    89 => "near_north_pole",
    1 => "near_positive_origin",
    -1 => "near_negative_origin",
    -89 => "near_south_pole",
    -90 => "south_pole",
  }
  if lng == 0
    return lng_desc[lng][lat]
  elsif lat == 0
    return lat_desc[lat][lng]
  else
    return lng_desc[lng] + "_" + lat_desc[lat]
  end
end

def get_filename(start_lng, start_lat, end_lng, end_lat)
  s = get_point(start_lng, start_lat)
  e = get_point(end_lng, end_lat)
  if s == e
    ret = "#{s}.test"
  else
    ret = "#{s}_to_#{e}.test"
  end
  ret
end



if __FILE__ == $0

  OPTS = {}
  parser = OptionParser.new
  parser.on("-g", "--generate-filename", "generate test file name") do |file_name|
    OPTS[:file_name] = file_name
  end
  parser.on("-t", "--generate-test", "generate .test file") do |test|
    OPTS[:test] = test
  end
  parser.on("-e", "--generate-expected", "generate .expeceted file") do |expected|
    OPTS[:expected] = expected
  end
  parser.on("-c CSV_FILE", "--csv CSV_FILE", "source CSV file") do |csv_file|
    OPTS[:csv] = csv_file
  end
  parser.on("-v", "--verbose", "show log in detail") do |verbose|
    OPTS[:verbose] = verbose
  end

  parser.parse!(ARGV)

  exit if not OPTS.has_key?(:csv)

  grndata = new GrnTestData(OPTS)

  File.open(OPTS[:csv], "r") do |csv_file|
    lines = csv_file.readlines

    lines.each_with_index do |line, i|
      is_header = i == 0
      next if is_header

      #puts "line No #{i}"

      grndata.parse_line_data(line)

      app_types = ["", "rectangle", "rect"]
      app_types = [""]

      quadrant = get_quadrant_to(lng_sdeg, lat_sdeg, lng_edeg, lat_edeg)

      prefix = long?(lng_sdeg, lng_edeg) ? "long" : "short"

      type = point?(lng_sdeg, lat_sdeg, lng_edeg, lat_edeg) ? "point" : "line"

      if OPTS.has_key?(:file_name)
        filename = get_filename(lng_sdeg, lat_sdeg, lng_edeg, lat_edeg)

        puts "#{line.chomp}"
        # show new generated filename
        puts line.chomp.split(",")[0..-2].join(",") + ",#{prefix}/#{quadrant}/#{type}/#{filename}"

      elsif OPTS.has_key?(:test)
        app_types.each do |app_type|
          scorer = ""
          file_prefix = ""
          select_postfix = ""
          comment = sprintf("# from (longitude %s latitude %s) to (longitude %s latitude %s)\n",
                    lng_sdeg, lat_sdeg, lng_edeg, lat_edeg)
          scorer = sprintf("--scorer 'distance = geo_distance(\"%sx%s\", \"%sx%s\"",
                   lng_start, lat_start, lat_end, lng_end, app_type)
          if app_type == ""
            # default
            select_postfix = ")'\n"
          else
            file_prefix = app_type + "_"
            select_postfix = ", \"#{app_type}\")'\n"
          end
          dottest = sprintf("%s%s\n%s\n%s%s%s%s",
                    TABLE_CREATE,
                    COLUMN_CREATE,
                    LOAD,
                    comment,
                    SELECT, scorer, select_postfix)

          if filename and filename != ""
            testname = sprintf("%s/%s/%s/%s%s",
                       prefix, quadrant, type,
                       file_prefix, File.basename(filename))
          else
            exit 1
          end

          if testname and not Dir.exists?(File.dirname(testname))
            FileUtils.mkdir_p(File.dirname(testname))
          end

          if File.exists?(testname)
            # duplicated?
            puts "Warning! #{testname} duplicated"
          end
          File.open(testname, "w+") do |testfile|
            if OPTS.has_key?(:verbose)
              puts testname
              puts dottest
            end
            testfile.puts(dottest)
          end
        end
      end
    end
  end
end

