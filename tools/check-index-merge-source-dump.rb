#!/usr/bin/env ruby
#
# Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>
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

require "English"

require "groonga-log"

def flatten_record_batches(record_batches)
  record_batches.inject([], &:<<)
end

parser = GroongaLog::Parser.new

n_sections = 5
record_batches = []
record_batch = []
contexts = []
term = nil
term_id = nil
parser.parse(ARGF) do |statistics|
  message = statistics.message
  case statistics.message
  when /\A\[ii\]\[merge\]\[source\]\[[a-zA-Z0-9.]+\]\[\d+\/\d+\]
          \ <"(.*?)">\((\d)\):\ /x
    current_term = $1
    current_term_id = $2.to_i
    context = $POSTMATCH
    if record_batch != record_batch.sort
      pp([:invalid_current_records,
          term, term_id,
          record_batches.collect(&:size),
          record_batch.size,
          contexts])
      pp({record_batch: record_batch})
      exit(false)
    end
    record_batches << record_batch unless record_batch.empty?
    sorted_record_batches = record_batches.sort_by do |batch|
      batch[0]
    end
    if flatten_record_batches(sorted_record_batches) !=
       flatten_record_batches(record_batches).sort
      pp([:invalid_records,
          term, term_id,
          record_batches.collect(&:size),
          record_batch.size,
          contexts])
      pp({sorted_record_batches: sorted_record_batches.flatten,
          record_batch: record_batch})
      exit(false)
    end
    if term_id != current_term_id
      expected_record_id = 1
      expected_section_id = 1
      sorted_record_batches.each do |batch|
        batch.each do |record_id, section_id, _, _|
          if [record_id, section_id] != [expected_record_id, expected_section_id]
            pp([:invalid,
                term, term_id,
                [record_id, section_id],
                [expected_record_id, expected_section_id],
                record_batches.collect(&:size),
                batch.size,
                contexts])
            pp({sorted_record_batches: sorted_record_batches,
                record_batch: batch})
            exit(false)
          end
          if expected_section_id == n_sections
            expected_record_id += 1
            expected_section_id = 1
          else
            expected_section_id += 1
          end
        end
      end
      pp([:checked,
          term, term_id,
          record_batches.collect(&:size),
          contexts])
      record_batches = []
      contexts = []
      term = current_term
      term_id = current_term_id
    end
    record_batch = []
    contexts << context
  when /\Arecord:/
    record = message.scan(/\((\d+):(\d+):(\d+):(\d+)\)/)[0].collect(&:to_i)
    record_batch << record
  when /\A\(/
    message.scan(/\((\d+):(\d+):(\d+):(\d+)\)/) do |raw_record|
      record_batch << raw_record.collect(&:to_i)
    end
  else
  end
end
