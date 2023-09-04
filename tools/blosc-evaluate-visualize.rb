#!/usr/bin/env ruby
#
# Copyright (C) 2023  Sutou Kouhei <kou@clear-code.com>
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

require "charty"

approaches = []
compress_ratios = []
compress_times = []

def normalize_approach(approach)
  approach.gsub(/RecordBatch|Record| \+ shuffle| \+ bytedelta/) do |matched|
    case matched
    when "RecordBatch"
      "All: "
    when "Record"
      "Record: "
    when " + shuffle"
      "+S"
    when " + bytedelta"
      "+D"
    end
  end
end

ARGF.each_line(chomp: true) do |line|
  case line
  when /\A(.+): Total: Compress: \d+: ([0-9.]+)%: ([0-9.]+)s\z/
    approach = $1
    compress_ratio = (100 - $2.to_f)
    compress_time = $3.to_f
    approaches << normalize_approach(approach)
    compress_ratios << compress_ratio
    compress_times << compress_time
  end
end

Charty::Backends.use("pyplot")
{
  "zstd" => /zstd:/,
  "lz4" => /lz4:/,
  "zstd-shuffle" => /zstd\+S:/,
  "lz4-shuffle" => /lz4\+S:/,
  "zstd-shuffle-bytedelta" => /zstd\+S\+D:/,
  "lz4-shuffle-bytedelta" => /lz4\+S\+D:/,
  "record-sort" => /Record: Sort/,
  "record-min-shift" => /Record: MinShift/,
  "record-normalize" => /Record: Normalize/,
  "all-sort" => /All: Sort/,
  "all-min-shift" => /All: MinShift/,
  "all-normalize" => /All: Normalize/,
}.each do |name, pattern|
  target_indices = []
  approaches.each_with_index do |approach, i|
    target_indices << i if pattern.match?(approach)
  end
  data = {
    "Approach" => approaches.values_at(*target_indices),
    "Compress ratio (%, higher is better)" => compress_ratios.values_at(*target_indices),
    "Compress time (sec, smaller is better)" => compress_times.values_at(*target_indices),
  }
  plotter = Charty.scatter_plot(data: data,
                                x: "Compress time (sec, smaller is better)",
                                y: "Compress ratio (%, higher is better)",
                              color: "Approach")
  plotter.save("blosc-evaluate-#{name}.svg")
end
