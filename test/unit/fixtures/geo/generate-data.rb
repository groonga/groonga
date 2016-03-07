#!/usr/bin/env ruby

seed_latitude = (45 * 60 * 60 + 0 * 60 + 0) * 1000
seed_longitude = (90 * 60 * 60 + 0 * 60 + 0) * 1000

n_places = 32
common_n_places = 28

rest_n_places = n_places - common_n_places

seed_latitude_in_string = ("%0#{n_places}b" % seed_latitude)
seed_longitude_in_string = ("%0#{n_places}b" % seed_longitude)

base_latitude_in_string = seed_latitude_in_string[0, common_n_places]
base_latitude_in_string += "0" * rest_n_places
base_longitude_in_string = seed_longitude_in_string[0, common_n_places]
base_longitude_in_string += "0" * rest_n_places

base_latitude = base_latitude_in_string.to_i(2)
base_longitude = base_longitude_in_string.to_i(2)

points = []
max_variable_value = rest_n_places ** 2
max_variable_value.times do |i|
  latitude = base_latitude + i
  max_variable_value.times do |j|
    longitude = base_longitude + j
    points << [latitude, longitude]
  end
end

def to_degree(point)
  "(%d, %d, %d, %d)" % [(point / 1000 / 60 / 60),
                        (point / 1000 / 60) % 60,
                        (point / 1000) % 60,
                        (point % 1000)]
end

puts("load --table Points")
puts("[")
puts("[\"_key\", \"short_key\", \"degree\", \"short_degree\", \"location\"],")
points.each do |latitude, longitude|
  latitude_in_string = "%0#{n_places}b" % latitude
  longitude_in_string = "%0#{n_places}b" % longitude
  short_key = ""
  rest_n_places.times do |i|
    short_key << latitude_in_string[i]
    short_key << longitude_in_string[i]
  end
  latitude_in_degree = to_degree(latitude)
  longitude_in_degree = to_degree(longitude)
  degree = "(#{latitude_in_degree},#{longitude_in_degree})"
  short_degree_n_places = (Math.log10(max_variable_value) + 1).truncate
  short_degree_format = "(%0#{short_degree_n_places}d," +
                         "%0#{short_degree_n_places}d)"
  short_degree = short_degree_format % [latitude % max_variable_value,
                                        longitude % max_variable_value]

  print("[")
  print("\"0b#{latitude_in_string}\\n")
  print("0b#{longitude_in_string}\",")
  print("\"#{short_key}\",")
  print("\"#{degree}\",")
  print("\"#{short_degree}\",")
  print("\"#{latitude}x#{longitude}\"")
  puts("],")
end
puts("]")
