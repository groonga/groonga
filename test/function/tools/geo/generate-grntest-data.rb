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

SELECT_PRE = "[[0,0.0,0.0],[[[1],[[\"distance\",\"Int32\"]],["
SELECT_POST = "]]]]"

GRN_GEO_RESOLUTION=3600000
GRN_GEO_RADIUS=6357303

class GrnTestData
  attr_accessor :csv_file
  attr_accessor :options

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

  def initialize(options={})
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

  def quadrant_point(longitude, latitude)
    if longitude > 0 and latitude > 0
      "1st"
    elsif longitude < 0 and latitude > 0
      "2nd"
    elsif longitude < 0 and latitude < 0
      "3rd"
    elsif longitude > 0 and latitude < 0
      "4th"
    else
      nil
    end
  end

  def east_axis_to_north_axis?
    east_axis?(@longitude_start_degree, @latitude_start_degree) and
      north_axis?(@longitude_end_degree, @latitude_end_degree)
  end

  def north_axis_to_east_axis?
    north_axis?(@longitude_start_degree, @latitude_start_degree) and
      east_axis?(@longitude_end_degree, @latitude_end_degree)
  end

  def north_axis_to_west_axis?
    north_axis?(@longitude_start_degree, @latitude_start_degree) and
      west_axis?(@longitude_end_degree, @latitude_end_degree)
  end

  def west_axis_to_north_axis?
    west_axis?(@longitude_start_degree, @latitude_start_degree) and
      north_axis?(@longitude_end_degree, @latitude_end_degree)
  end

  def west_axis_to_south_axis?
    west_axis?(@longitude_start_degree, @latitude_start_degree) and
      south_axis?(@longitude_end_degree, @latitude_end_degree)
  end

  def quadrant_point_with_axis(quadrant, longitude, latitude)
    case quadrant
    when "1st"
      if longitude >= 0 and latitude >= 0
        "1st"
      else
        nil
      end
    when "2nd"
      if longitude <= 0 and latitude >= 0
        "2nd"
      else
        nil
      end
    when "3rd"
      if longitude <= 0 and latitude <= 0
        "3rd"
      else
        nil
      end
    when "4th"
      if longitude >= 0 and latitude <= 0
        "4th"
      else
        nil
      end
    end
  end

  def south_axis_to_west_axis?
    south_axis?(@longitude_start_degree, @latitude_start_degree) and
      west_axis?(@longitude_end_degree, @latitude_end_degree)
  end

  def east_axis_to_south_axis?
    east_axis?(@longitude_start_degree, @latitude_start_degree) and
            south_axis?(@longitude_end_degree, @latitude_end_degree)
  end

  def south_axis_to_east_axis?
    south_axis?(@longitude_start_degree, @latitude_start_degree) and
      east_axis?(@longitude_end_degree, @latitude_end_degree)
  end


  def within_specified_quadrant?(quadrant)
    start_quadrant = quadrant_point_with_axis(quadrant,
                                              @longitude_start_degree,
                                              @latitude_start_degree)
    end_quadrant = quadrant_point_with_axis(quadrant,
                                            @longitude_end_degree,
                                            @latitude_end_degree)
    quadrant == start_quadrant and quadrant == end_quadrant
  end

  def quadrant
    squadrant = quadrant_point(@longitude_start_degree, @latitude_start_degree)
    equadrant = quadrant_point(@longitude_end_degree, @latitude_end_degree)
    # p squadrant
    # p equadrant
    # p start_lng
    # p start_lat
    # p end_lng
    # p end_lat
    if (@longitude_start_degree == @longitude_end_degree and
        @longitude_start_degree == 0)
      "meridian"
    elsif (@latitude_start_degree == @latitude_end_degree and
        @latitude_start_degree == 0)
      "equator"
    elsif !squadrant or !equadrant
      if (not squadrant) and (not equadrant)
        if east_axis_to_north_axis? or north_axis_to_east_axis?
          "1st"
        elsif north_axis_to_west_axis? or west_axis_to_north_axis?
          "2nd"
        elsif west_axis_to_south_axis? or south_axis_to_west_axis?
          "3rd"
        elsif east_axis_to_south_axis? or south_axis_to_east_axis?
          "4th"
        end
      elsif not squadrant
        equadrant
      elsif not equadrant
        squadrant
      end
    else
      if squadrant == equadrant
        equadrant
      else
        "#{squadrant}to#{equadrant}"
      end
    end
  end

  def type_of_diff_in_longitude
    long?(@longitude_start_degree, @longitude_end_degree) ? "long" : "short"
  end

  def long?(start_lng_deg, end_lng_deg)
    diff_in_longitude = start_lng_deg.abs + end_lng_deg.to_i.abs
    east_to_west = start_lng_deg > 0 and end_lng_deg.to_i < 0
    west_to_east = start_lng_deg < 0 and end_lng_deg.to_i > 0
    if start_lng_deg != end_lng_deg and
        (east_to_west or west_to_east) and
        diff_in_longitude > 180
      true
    else
      false
    end
  end

  def point_or_line
    point?(@longitude_start_degree, @latitude_start_degree,
           @longitude_end_degree, @latitude_end_degree) ? "point" : "line"
  end

  def point?(start_longitude, start_latitude, end_longitude, end_latitude)
    start_longitude == end_longitude and start_latitude == end_latitude
  end

  def longitude_equal?
    @longitude_start_degree == @longitude_end_degree
  end

  def latitude_equal?
    @latitude_start_degree == @latitude_end_degree
  end

  def to_north?(check_option=nil)
    check_option ||= {:check_longitude => true}
    if check_option[:check_longitude] == true
      longitude_equal? and @latitude_start_degree < @latitude_end_degree
    else
      @latitude_start_degree < @latitude_end_degree
    end
  end

  def to_east?(check_option=nil)
    check_option ||= {:check_latitude => true}
    if check_option[:check_latitude] == true
      latitude_equal? and @longitude_start_degree < @longitude_end_degree
    else
      @longitude_start_degree < @longitude_end_degree
    end
  end

  def to_west?(check_option=nil)
    check_option ||= {:check_latitude => true}
    if check_option[:check_latitude] == true
      latitude_equal? and @longitude_start_degree > @longitude_end_degree
    else
      @longitude_start_degree > @longitude_end_degree
    end
  end

  def to_south?(check_option=nil)
    check_option ||= {:check_longitude => true}
    if check_option[:check_longitude] == true
      longitude_equal? and @latitude_start_degree > @latitude_end_degree
    else
      @latitude_start_degree > @latitude_end_degree
    end
  end

  def to_north_east?
    check_longitude = {:check_longitude => false}
    check_latitude = {:check_latitude => false}
    to_north?(check_longitude) and to_east?(check_latitude)
  end

  def to_north_west?
    check_longitude = {:check_longitude => false}
    check_latitude = {:check_latitude => false}
    to_north?(check_longitude) and to_west?(check_latitude)
  end

  def to_south_east?
    check_longitude = {:check_longitude => false}
    check_latitude = {:check_latitude => false}
    to_south?(check_longitude) and to_east?(check_latitude)
  end

  def to_south_west?
    check_longitude = {:check_longitude => false}
    check_latitude = {:check_latitude => false}
    to_south?(check_longitude) and to_west?(check_latitude)
  end

  def direction
    is_point = point?(@longitude_start_degree, @latitude_start_degree,
                      @longitude_end_degree, @latitude_end_degree)
    if is_point
      ""
    else
      if within_specified_quadrant?("1st") or
         within_specified_quadrant?("2nd") or
         within_specified_quadrant?("3rd") or
         within_specified_quadrant?("4th")
        if to_north?
          "north"
        elsif to_east?
          "east"
        elsif to_west?
          "west"
        elsif to_south?
          "south_west"
        elsif to_north_east?
          "north_east"
        elsif to_north_west?
          "north_west"
        elsif to_south_east?
          "south_east"
        elsif to_south_west?
          "south_west"
        end
      end
    end
  end

  def generate_filename
    "#{latitude_position}.test"
  end

  def longitude_position
    longitude_desc = {
      180 => "on_180_degrees",
      179 => "almost_180_degrees",
      91 => "almost_90_degrees_larger",
      90 => "on_90_degrees",
      89 => "almost_90_degrees_smaller",
      1 => "almost_0_degree_larger",
      0 => "on_0_degree",
      -1 => "almost_0_degree_smaller",
      -89 => "almost_-90_degrees_larger",
      -90 => "on_-90_degrees",
      -91 => "almost_-90_degrees_smaller",
      -179 => "almost_-180_degrees",
      -180 => "on_-180_degrees",
    }
    if longitude_equal?
      longitude_desc[@longitude_start_degree]
    else
      start_degree = longitude_desc[@longitude_start_degree]
      end_degree = longitude_desc[@longitude_end_degree]
      "#{start_degree}_to_#{end_degree}"
    end
  end

  def latitude_position
    latitude_desc = {
      90 => "on_90_degrees",
      89 => "almost_90_degrees_smaller",
      1 => "almost_0_degree_larger",
      0 => "on_0_degree",
      -1 => "almost_0_degree_smaller",
      -89 => "almost_-90_degrees_larger",
      -90 => "on_-90_degrees",
    }
    if latitude_equal?
      latitude_desc[@latitude_start_degree]
    else
      start_degree = latitude_desc[@latitude_start_degree]
      end_degree = latitude_desc[@latitude_end_degree]
      "#{start_degree}_to_#{end_degree}"
    end
  end

  def generate_new_data(line, prefix, quadrant, type, filename)
    geo_data = line.chomp.split(",")[0..-2].join(",")
    path = ",#{prefix}/#{quadrant}/#{type}/#{filename}"
    geo_data + path
  end

  def generate_test_data(app_type)
    select_postfix = ""
    comment = sprintf("# from (%s %s %s %s) to (%s %s %s %s)\n",
                      "longitude", @longitude_start_degree,
                      "latitude", @latitude_start_degree,
                      "longitude", @longitude_end_degree,
                      "latitude", @latitude_end_degree)
    scorer = sprintf("--scorer 'distance = geo_distance(\"%sx%s\", \"%sx%s\"",
                     @latitude_start, @longitude_start,
                     @latitude_end, @longitude_end)
    if app_type == ""
      # default
      select_postfix = ")'\n"
    else
      select_postfix = ", \"#{app_type}\")'\n"
    end
    sprintf("%s%s\n%s\n%s%s%s%s",
            TABLE_CREATE,
            COLUMN_CREATE,
            LOAD,
            comment,
            SELECT, scorer, select_postfix)
  end

  def geo_int2rad(value)
    ((Math::PI / (GRN_GEO_RESOLUTION * 180)) * (value))
  end

  def geo_distance(app_type)
    case app_type
    when "", "rect", "rectangle"
      lat1 = geo_int2rad(@latitude_start_degree)
      lng1 = geo_int2rad(@longitude_start_degree)
      lat2 = geo_int2rad(@latitude_end_degree)
      lng2 = geo_int2rad(@longitude_end_degree)
      x = (lng2 - lng1) * Math.cos((lat1 + lat2) * 0.5)
      y = (lat2 - lat1)
      return (Math.sqrt((x * x) + (y * y)) * GRN_GEO_RADIUS).floor
    end
  end

  def generate_expected_data(app_type)
    select_postfix = ""
    scorer = sprintf("--scorer 'distance = geo_distance(\"%sx%s\", \"%sx%s\"",
                     @latitude_start, @longitude_start,
                     @latitude_end, @longitude_end)
    if app_type == ""
      select_postfix = ")'\n"
    else
      select_postfix = ", \"#{app_type}\")'\n"
    end
    distance = geo_distance(app_type) unless @distance != ""
    [
      TABLE_CREATE,
      CREATE_RESULT,
      COLUMN_CREATE,
      CREATE_RESULT,
      LOAD,
      LOAD_RESULT,
      SELECT,
      scorer,
      select_postfix,
      SELECT_PRE,
      distance,
      SELECT_POST
    ].join
  end
end


def get_point(lng, lat)
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
    lng_desc[lng][lat]
  elsif lat == 0
    lat_desc[lat][lng]
  else
    lng_desc[lng] + "_" + lat_desc[lat]
  end
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
  parser.on("-d", "--generate-filename-with-csv-data",
            "generate test file name with geo data") do |csv_data|
    OPTS[:csv_data] = csv_data
  end
  parser.on("--quadrant-with-axis QUADRANT",
            "extract QUADRANT including axis data") do |quadrant_with_axis|
    OPTS[:quadrant_with_axis] = quadrant_with_axis
  end
  parser.on("-v", "--verbose", "show log in detail") do |verbose|
    OPTS[:verbose] = verbose
  end

  parser.parse!(ARGV)

  exit if not OPTS.has_key?(:csv)

  grndata = GrnTestData.new(OPTS)

  File.open(OPTS[:csv], "r") do |csv_file|
    lines = csv_file.readlines

    lines.each_with_index do |line, i|
      is_header = i == 0
      next if is_header

      #puts "line No #{i}"

      grndata.parse_line_data(line)

      if OPTS.has_key?(:quadrant_with_axis)
        next unless grndata.within_specified_quadrant?(OPTS[:quadrant_with_axis])
      end

      app_types = ["", "rectangle", "rect", "sphere", "sphr", "ellipsoid", "ellip"]

      quadrant = grndata.quadrant

      type_longitude = grndata.type_of_diff_in_longitude

      type = grndata.point_or_line

      direction = grndata.direction

      longitude_position = grndata.longitude_position

      filename = grndata.generate_filename

      if OPTS.has_key?(:file_name)

        if OPTS.has_key?(:verbose)
          puts(line)
        end
        if type == "line"
          path = File.join(type_longitude,
                           quadrant,
                           type,
                           direction,
                           longitude_position,
                           filename)
          puts(path)
        else
          path = File.join(type_longitude,
                           quadrant,
                           type,
                           longitude_position,
                           filename)
          puts(path)
        end
      elsif OPTS.has_key?(:csv_data)
        puts(grndata.generate_new_data(line, type_longitude, quadrant, type, filename))
      elsif OPTS.has_key?(:test) or OPTS.has_key?(:expected)
        app_types.each do |app_type|
          file_prefix = ""
          if app_type != ""
            file_prefix = app_type + "_"
          end

          dot_test = grndata.generate_test_data(app_type)

          if filename and filename != ""
            if type == "line"
              test_name = sprintf("%s/%s/%s/%s/%s/%s%s",
                                  type_longitude, quadrant, type, direction,
                                  longitude_position, file_prefix,
                                  File.basename(filename))
            else
              test_name = sprintf("%s/%s/%s/%s/%s%s",
                                  type_longitude, quadrant, type,
                                  longitude_position, file_prefix,
                                  File.basename(filename))
            end
          else
            exit 1
          end

          if test_name and not Dir.exists?(File.dirname(test_name))
            FileUtils.mkdir_p(File.dirname(test_name))
          end

          dot_expected = grndata.generate_expected_data(app_type)

          if File.exists?(test_name)
            # duplicated?
            puts("Warning! [#{i}] #{test_name} duplicated")
          end
          File.open(test_name, "w+") do |test_file|
            if OPTS.has_key?(:verbose)
              puts(test_name)
              puts(dot_test)
            end
            test_file.puts(dot_test)
          end
          expected_name = sprintf("%s/%s.expected",
                                  File.dirname(test_name),
                                  File.basename(test_name, ".test"))
          if OPTS.has_key?(:expected)
            File.open(expected_name, "w+") do |expected_file|
              if OPTS.has_key?(:verbose)
                puts(expected_name)
                puts(dot_expected)
              end
              expected_file.puts(dot_expected)
            end
          end
        end
      end
    end
  end
end
