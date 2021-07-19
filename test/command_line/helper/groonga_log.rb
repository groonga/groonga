# Copyright(C) 2019 Sutou Kouhei <kou@clear-code.com>
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

module GroongaLog
  def remove_timestamp(line)
    line.gsub(/\A\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{6}/, "")
  end

  def expected_groonga_log(level, messages)
    log_file = Tempfile.new("groonga-log")
    log_file.close
    groonga("status",
            command_line: [
              "--log-path", log_file.path,
              "--log-level", level,
            ])
    standard_log_lines = normalize_groonga_log(File.read(log_file.path)).lines
    log = ""
    standard_log_lines.each do |line|
      case remove_timestamp(line)
      when /\A\|n\| grn_init/,
           /\A\|n\| vm\.overcommit_memory/,
           /\A\|i\| Some processings with/,
           /\A\|i\| To set/,
           /\A\|i\| add/,
           /\A\|i\| run/,
           /\A\|[i-]\| \[io\]\[open\]/
        log << line
      else
        break
      end
    end
    unless messages.empty?
      messages.each_line do |message|
        next if message.chomp.empty?
        log << "1970-01-01 00:00:00.000000#{message}"
      end
    end
    last_log_lines = []
    standard_log_lines.reverse_each do |line|
      case remove_timestamp(line)
      when /\A\|[i-]\| \[io\]\[close\]/,
           /\A\|n\| grn_fin/
        last_log_lines << line
      else
        break
      end
    end
    log << last_log_lines.reverse.join("")
    log
  end

  def open_file_log_line(path, log_level)
    if windows?
      case log_level
      when "info",
           "debug",
           "dump"
        "|i| [io][open] open existing file: <#{path}>"
      else
        ""
      end
    else
      case log_level
      when "dump"
        "|-| [io][open] <#{path}>"
      else
        ""
      end
    end
  end

  def create_file_log_line(path, log_level)
    if windows?
      case log_level
      when "info",
           "debug",
           "dump"
        "|i| [io][open] create new file: <#{path}>"
      else
        ""
      end
    else
      case log_level
      when "dump"
        "|-| [io][open] <#{path}>"
      else
        ""
      end
    end
  end

  def close_file_log_line(path, log_level)
    if windows?
      case log_level
      when "info",
           "debug",
           "dump"
        "|i| [io][close] <#{path}>"
      else
        ""
      end
    else
      case log_level
      when "dump"
        "|-| [io][close] <#{path}>"
      else
        ""
      end
    end
  end

  def prepend_tag(tag, messages)
    messages.each_line.inject("") do |result, line|
      result + "#{tag}#{line}"
    end
  end

  def normalize_groonga_log_message(message)
    message = message.gsub(/grn_init: <.+?>/, "grn_init: <VERSION>")
    message = message.gsub(/(?:
                              system\ call\ error| # Linux
                              system\ error\[\d+\] # Windows
                            ):\ [^:]+:/x) do
      "system call error: DETAIL:"
    end
    # normalize temporary file generated by grn_mkstemp
    message = message.gsub(/<(
                                #{Regexp.escape(@database_path.to_s)}
                                \.\d{7}
                             )
                             [0-9a-zA-Z]{6}
                            >/x) do
      "<#{$1}XXXXXX>"
    end
    message = message.gsub(/<(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}):(\d{2})\.\d{6}>/) do
      "<%s:%02d.000000>" % [$1, Integer($2, 10) / 10 * 10]
    end
    message
  end

  def format_since(since)
    "%04d-%02d-%02dT%02d:%02d:%02d.000000" % [
      since.year,
      since.mon,
      since.day,
      since.hour,
      since.min,
      since.sec / 10 * 10,
    ]
  end

  def normalize_groonga_log_line(line)
    line.chomp.gsub(/\A
                       (\d{4}-\d{2}-\d{2}\ \d{2}:\d{2}:\d{2}\.\d+)?
                       \|\
                       ([a-zA-Z])
                       \|\
                       ([^: ]+)?
                       ([|:]\ )?
                       (.+)
                    \z/x) do
    end
  end

  def normalize_groonga_log(content)
    normalized = ""
    content.each_line do |line|
      case line.chomp
      when /\A
              (\d{4}-\d{2}-\d{2}\ \d{2}:\d{2}:\d{2}\.\d+)?
              \|\
              ([a-zA-Z-])
              \|\
              ([^: ]+)?
              ([|:]\ )?
              (.+)
            \z/x
        timestamp = $1
        level = $2
        id_section = $3
        separator = $4
        message = $5
        next if stack_trace_groonga_log_message?(message)

        timestamp = "1970-01-01 00:00:00.000000" if timestamp
        case id_section
        when nil
        when /\|/
          id_section = "PROCESS_ID|THREAD_ID"
        when /[a-zA-Z]/
          id_section = "THREAD_ID"
        when /\A\d{8,}\z/
          id_section = "THREAD_ID"
        else
          id_section = "PROCESS_ID"
        end
        message = normalize_groonga_log_message(message)
        normalized <<
          "#{timestamp}|#{level}|#{id_section}#{separator}#{message}\n"
      else
        normalized << line
      end
    end
    normalized
  end

  def remove_groonga_log
    FileUtils.rm(@log_path)
  end

  def groonga_log_content
    File.read(@log_path, encoding: "UTF-8")
  end

  def normalized_groonga_log_content
    normalize_groonga_log(groonga_log_content).encode("locale")
  end

  private
  def stack_trace_groonga_log_message?(message)
    case message.strip
    when /\A\//
      true
    when /\A[a-zA-Z]:[\/\\]/
      true
    when /\A\(unknown\):\d+:\d+: /
      true
    when /\A[\w.\\-]+:\d+:\d+: /
      true
    when /\Agroonga\(\) \[0x[\da-f]+\]\z/
      true
    when /\A\d+\s+(?:lib\S+\.dylib|\S+\.so|groonga|nginx|\?\?\?)\s+
             0x[\da-f]+\s
             \S+\s\+\s\d+\z/x
      true
    else
      false
    end
  end
end
