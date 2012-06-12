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

GRN_GEO_RESOLUTION = 3600000
GRN_GEO_RADIUS = 6357303

GEO_DISTANCE_1LONGITUDE = 111263

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

  def meridian?(longitude, latitude)
    longitude.zero?
  end

  def equator?(longitude, latitude)
    latitude.zero?
  end

  def east_axis?(longitude, latitude)
    longitude >= 0 and latitude.zero?
  end

  def west_axis?(longitude, latitude)
    longitude <= 0 and latitude.zero?
  end

  def north_axis?(longitude, latitude)
    longitude.zero? and latitude >= 0
  end

  def south_axis?(longitude, latitude)
    longitude.zero? and latitude <= 0
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
    if (@longitude_start_degree == @longitude_end_degree and
        @longitude_start_degree.zero?)
      "meridian"
    elsif (@latitude_start_degree == @latitude_end_degree and
           @latitude_start_degree.zero?)
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
        case equadrant
        when "1st"
          if north_axis?(@longitude_start_degree, @latitude_start_degree) or
             east_axis?(@longitude_start_degree, @latitude_start_degree)
            "1st"
          elsif west_axis?(@longitude_start_degree, @latitude_start_degree)
            "2nd_to_1st"
          elsif south_axis?(@longitude_start_degree, @latitude_start_degree)
            "4th_to_1st"
          end
        when "2nd"
          if north_axis?(@longitude_start_degree, @latitude_start_degree) or
             west_axis?(@longitude_start_degree, @latitude_start_degree)
            "2nd"
          elsif east_axis?(@longitude_start_degree, @latitude_start_degree)
            "1st_to_2nd"
          elsif south_axis?(@longitude_start_degree, @latitude_start_degree)
            "3rd_to_2nd"
          end
        when "3rd"
          if south_axis?(@longitude_start_degree, @latitude_start_degree) or
             west_axis?(@longitude_start_degree, @latitude_start_degree)
            "3rd"
          elsif east_axis?(@longitude_start_degree, @latitude_start_degree)
            "4th_to_3rd"
          elsif north_axis?(@longitude_start_degree, @latitude_start_degree)
            "2nd_to_3rd"
          end
        when "4th"
          if south_axis?(@longitude_start_degree, @latitude_start_degree) or
             east_axis?(@longitude_start_degree, @latitude_start_degree)
            "4th"
          elsif west_axis?(@longitude_start_degree, @latitude_start_degree)
            "3rd_to_4th"
          elsif north_axis?(@longitude_start_degree, @latitude_start_degree)
            "1st_to_4th"
          end
        end
      elsif not equadrant
        case squadrant
        when "1st"
          if north_axis?(@longitude_end_degree, @latitude_end_degree) or
             east_axis?(@longitude_end_degree, @latitude_end_degree)
            "1st"
          elsif west_axis?(@longitude_end_degree, @latitude_end_degree)
            "1st_to_2nd"
          elsif south_axis?(@longitude_end_degree, @latitude_end_degree)
            "1st_to_4th"
          end
        when "2nd"
          if north_axis?(@longitude_end_degree, @latitude_end_degree) or
             west_axis?(@longitude_end_degree, @latitude_end_degree)
            "2nd"
          elsif east_axis?(@longitude_end_degree, @latitude_end_degree)
            "2nd_to_1st"
          elsif south_axis?(@longitude_end_degree, @latitude_end_degree)
            "2nd_to_4th"
          end
        when "3rd"
          if west_axis?(@longitude_end_degree, @latitude_end_degree) or
             south_axis?(@longitude_end_degree, @latitude_end_degree)
            "3rd"
          elsif north_axis?(@longitude_end_degree, @latitude_end_degree)
            "3rd_to_2nd"
          elsif east_axis?(@longitude_end_degree, @latitude_end_degree)
            "3rd_to_4th"
          end
        when "4th"
          if east_axis?(@longitude_end_degree, @latitude_end_degree) or
             south_axis?(@longitude_end_degree, @latitude_end_degree)
            "4th"
          elsif north_axis?(@longitude_end_degree, @latitude_end_degree)
            "4th_to_1st"
          elsif west_axis?(@longitude_end_degree, @latitude_end_degree)
            "4th_to_3rd"
          end
        end
      end
    else
      if squadrant == equadrant
        equadrant
      else
        "#{squadrant}_to_#{equadrant}"
      end
    end
  end

  def type_of_diff_in_longitude
    long?(@longitude_start_degree, @longitude_end_degree) ? "long" : "short"
  end

  def diff_in_longitude(start_degree, end_degree)
    if start_degree >= 0
      (start_degree - end_degree).abs
    else
      (end_degree - start_degree).abs
    end
  end

  def long?(start_longitude, end_longitude)
    longitude_diff = diff_in_longitude(start_longitude, end_longitude)
    east_to_west = start_longitude > 0 and end_longitude < 0
    west_to_east = start_longitude < 0 and end_longitude > 0
    if start_longitude != end_longitude and
        (east_to_west or west_to_east) and
        longitude_diff > 180
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
    if check_option[:check_longitude]
      longitude_equal? and @latitude_start_degree < @latitude_end_degree
    else
      @latitude_start_degree < @latitude_end_degree
    end
  end

  def to_east?(check_option=nil)
    check_option ||= {:check_latitude => true}
    if check_option[:check_latitude]
      latitude_equal? and @longitude_start_degree < @longitude_end_degree
    else
      @longitude_start_degree < @longitude_end_degree
    end
  end

  def to_west?(check_option=nil)
    check_option ||= {:check_latitude => true}
    if check_option[:check_latitude]
      latitude_equal? and @longitude_start_degree > @longitude_end_degree
    else
      @longitude_start_degree > @longitude_end_degree
    end
  end

  def to_south?(check_option=nil)
    check_option ||= {:check_longitude => true}
    if check_option[:check_longitude]
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
         within_specified_quadrant?("4th") or
         quadrant == "1st_to_2nd"
         quadrant == "2nd_to_1st"
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

  def calculate_to_180_degree(longitude, latitude)
    latitude_start = geo_int2rad(latitude)
    longitude_start = geo_int2rad(longitude)
    latitude_end = geo_int2rad(latitude)
    longitude_end = geo_int2rad(180 * GRN_GEO_RESOLUTION)
    longitude_diff = (longitude_end - longitude_start)
    latitude_sum = (latitude_start + latitude_end)
    xdistance = longitude_diff * Math.cos(latitude_sum * 0.5)
    (xdistance * GRN_GEO_RADIUS).floor
  end

  def calculate_distance(longitude_start, latitude_start,
                         longitude_end, latitude_end)
    start_latitude = geo_int2rad(latitude_start.abs)
    start_longitude = geo_int2rad(longitude_start.abs)
    end_latitude = geo_int2rad(latitude_end.abs)
    end_longitude = geo_int2rad(longitude_end.abs)
    longitude_diff = (end_longitude - start_longitude)
    x = longitude_diff * Math.cos((start_latitude + end_latitude) * 0.5)
    y = (end_latitude - start_latitude)
    Math.sqrt((x * x) + (y * y)) * GRN_GEO_RADIUS
  end

  def geo_distance(app_type)
    case app_type
    when "", "rect", "rectangle"
      if type_of_diff_in_longitude == "short"
        case quadrant
        when "1st_to_2nd", "2nd_to_1st"
          longitude_delta = @longitude_end_degree - @longitude_start_degree
          latitude_delta = @latitude_end_degree - @latitude_start_degree
          slope = latitude_delta / longitude_delta.to_f
          intercept = @latitude_start_degree - slope * @longitude_start_degree
          east_distance = calculate_distance(@longitude_start.to_i,
                                             @latitude_start.to_i,
                                             0,
                                             intercept * GRN_GEO_RESOLUTION)
          west_distance = calculate_distance(0,
                                             intercept * GRN_GEO_RESOLUTION,
                                             @longitude_end.to_i,
                                             @latitude_end.to_i)
          (east_distance + west_distance).floor
        else
          calculate_distance(@longitude_start.to_i,
                             @latitude_start.to_i,
                             @longitude_end.to_i,
                             @latitude_end.to_i).floor
        end
      else
        if @latitude_start_degree == @latitude_end_degree
          east_distance = calculate_to_180_degree(@longitude_start.to_i.abs,
                                                  @latitude_start.to_i.abs)
          west_distance = calculate_to_180_degree(@longitude_end.to_i.abs,
                                                  @latitude_end.to_i.abs)
          (east_distance + west_distance).floor
        else
          case quadrant
          when "1st_to_2nd"
            rounded_longitude = @longitude_end_degree + 360
            rounded_latitude = @latitude_end_degree
            longitude_delta = rounded_longitude - @longitude_start_degree
            latitude_delta = rounded_latitude - @latitude_start_degree
            slope = latitude_delta / longitude_delta.to_f
            intercept = @latitude_start_degree - slope * @longitude_start_degree
            latitude_on_180 = slope * 180 + intercept
            east_distance = calculate_distance(@longitude_start.to_i,
                                               @latitude_start.to_i,
                                               180 * GRN_GEO_RESOLUTION,
                                               latitude_on_180 * GRN_GEO_RESOLUTION)
            west_distance = calculate_distance(@longitude_end.to_i,
                                               @latitude_end.to_i,
                                               180 * GRN_GEO_RESOLUTION,
                                               latitude_on_180 * GRN_GEO_RESOLUTION)
            (east_distance + west_distance).floor
          when "2nd_to_1st"
            rounded_longitude = @longitude_start_degree + 360
            rounded_latitude = @latitude_start_degree
            longitude_delta = @longitude_end_degree - rounded_longitude
            latitude_delta = @latitude_end_degree - rounded_latitude
            slope = latitude_delta / longitude_delta.to_f
            intercept = @latitude_end_degree - slope * @longitude_end_degree
            latitude_on_180 = slope * 180 + intercept
            east_distance = calculate_distance(@longitude_end.to_i,
                                               @latitude_end.to_i,
                                               180 * GRN_GEO_RESOLUTION,
                                               latitude_on_180 * GRN_GEO_RESOLUTION)
            west_distance = calculate_distance(@longitude_start.to_i,
                                               @latitude_start.to_i,
                                               180 * GRN_GEO_RESOLUTION,
                                               latitude_on_180 * GRN_GEO_RESOLUTION)
            (east_distance + west_distance).floor
          end
        end
      end
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
    if @distance != ""
      distance = @distance
    else
      distance = geo_distance(app_type)
    end
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

  def output_file(mode, file_name, file_content, line_no, line_data)
    if OPTS.has_key?(mode)
      if File.exists?(file_name)
        puts("Warning! [#{line_no}] #{file_name} duplicated [#{line_data.chomp}]")
      end
      File.open(file_name, "w+") do |file|
        if OPTS.has_key?(:verbose)
          puts(file_name)
          puts(file_content)
        end
        file.puts(file_content)
      end
    end
  end

  def parse_distance(file_name)
    if File.exists?(file_name)
      File.open(file_name, "r") do |file|
        data = file.read
        if data =~ /.*,\[(\d+)\]\]\]\]\n$/
          $1.to_i
        else
          raise "failed to parse the value of distance"
        end
      end
    end
  end

  def parse_distance_test_data(file_name)
    if File.exists?(file_name)
      File.open(file_name, "r") do |file|
        data = file.read
        if data =~ /# from \((.+)\) to \((.+)\)/
          start_degree = $1
          end_degree = $2
          if start_degree =~ /longitude (.+) latitude (.+)/
            @longitude_start_degree = $1.to_i
            @latitude_start_degree = $2.to_i
          else
            raise "failed to parse start point data"
          end
          if end_degree =~ /longitude (.+) latitude (.+)/
            @longitude_end_degree = $1.to_i
            @latitude_end_degree = $2.to_i
          else
            raise "failed to parse end point data"
          end
          @longitude_start = @longitude_start_degree * GRN_GEO_RESOLUTION
          @longitude_end = @longitude_end_degree * GRN_GEO_RESOLUTION
          @latitude_start = @latitude_start_degree * GRN_GEO_RESOLUTION
          @latitude_end = @latitude_end_degree * GRN_GEO_RESOLUTION
        else
          raise "failed to parse comment parts"
        end
      end
    end
  end

  def check_rejected
    Dir.chdir(OPTS[:check_reject]) do
      Dir.glob("**/*.reject") do |reject_file|
        directory = File.dirname(reject_file)
        basename = File.basename(reject_file, ".reject")
        expected_file = File.join(directory, "#{basename}.expected")
        test_file = File.join(directory, "#{basename}.test")
        actual_distance = parse_distance(reject_file)
        expected_distance = parse_distance(expected_file)
        parse_distance_test_data(test_file)

        distance_diff = actual_distance - expected_distance
        if distance_diff.abs > GEO_DISTANCE_1LONGITUDE
          pathdata = "test:#{directory}/#{basename}.test\n"
          posdata = sprintf("(%s %d %s %d) to (%s %d %s %d)",
                            "longitude", @longitude_start_degree,
                            "latitude", @latitude_start_degree,
                            "longitude", @longitude_end_degree,
                            "latitude", @latitude_end_degree)
          longitude_diff = (@longitude_end_degree - @longitude_start_degree).abs
          about_distance = longitude_diff * GEO_DISTANCE_1LONGITUDE
          data = sprintf("%s# %s\n# about:%d actual:%d expected:%d diff:%d",
                         pathdata, posdata, about_distance,
                         actual_distance, expected_distance,
                         distance_diff)
          puts(data)
        end
      end
    end
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
  parser.on("--check-reject DIRECTORY",
            "check .reject and .expected in detail") do |directory|
    OPTS[:check_reject] = directory
  end

  parser.parse!(ARGV)

  exit if not OPTS.has_key?(:csv)

  grndata = GrnTestData.new(OPTS)

  grndata.check_rejected if OPTS.has_key?(:check_reject)

  File.open(OPTS[:csv], "r") do |csv_file|
    lines = csv_file.readlines

    lines.each_with_index do |line, i|
      is_header = i.zero?
      next if is_header

      #puts "line No #{i}"

      grndata.parse_line_data(line)

      if OPTS.has_key?(:quadrant_with_axis)
        next unless grndata.quadrant == OPTS[:quadrant_with_axis]
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

          if test_name
            FileUtils.mkdir_p(File.dirname(test_name))
          end

          dot_expected = grndata.generate_expected_data(app_type)
          grndata.output_file(:test, test_name, dot_test, i, line)

          expected_name = sprintf("%s/%s.expected",
                                  File.dirname(test_name),
                                  File.basename(test_name, ".test"))
          grndata.output_file(:expected, expected_name, dot_expected, i, line)
        end
      end
    end
  end
end
