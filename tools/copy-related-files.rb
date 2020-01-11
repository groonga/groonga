#!/usr/bin/env ruby

require "fileutils"
require "optparse"
require "json"

class CopyRelatedFiles
  def initialize
    @database_path = nil
    @groonga = "groonga"
    @destination_path = "."
    @targets = []
  end

  def run(argv)
    return false unless parse_options(argv)

    ensure_schema
    copy

    true
  end

  private
  def parse_options(argv)
    parser = OptionParser.new
    parser.banner += " DATABASE_PATH"
    parser.on("--groonga=PATH",
              "The path to the groonga command.",
              "#{@grronga}") do |path|
      @groonga = path
    end
    parser.on("--destination=PATH",
              "The path to the destination directory.",
              "#{@destination_path}") do |path|
      @destination_path = path
    end
    parser.on("--target=TARGET",
              "The target table/column to be copied.",
              "You can specify this option multiple times",
              "to copy multiple tables/columns.") do |target|
      @targets << target
    end

    @database_path, = parser.parse!(argv)
    not @database_path.nil?
  end

  def ensure_schema
    response = ""
    IO.pipe do |input, output|
      pid = spawn(@groonga,
                  @database_path,
                  "schema",
                  "--output_pretty", "yes",
                  :out => output)
      output.close
      input.each_line do |line|
        response << line
      end
      Process.waitpid(pid)
    end
    @schema = JSON.parse(response)[1]
  end

  def copy
    FileUtils.mkdir_p(@destination_path)
    copy_builtin_files
    copy_target_files
  end

  def copy_builtin_files
    suffixes = [
      "",
      ".001",
      ".conf",
      ".options",
      ".0000000",
    ]
    suffixes.each do |suffix|
      FileUtils.cp("#{@database_path}#{suffix}",
                   @destination_path)
    end
  end

  def copy_target_files
    target_ids.each do |id|
      FileUtils.cp(Dir.glob("%s.%07X*" % [@database_path, id]),
                   @destination_path)
    end
  end

  def target_ids
    ids = []
    processed_targets = {}
    @targets.each do |target|
      table_name, column_name = target.split(".", 2)
      table = @schema["tables"][table_name]
      ids.concat(extract_ids_from_table(table, processed_targets))
      next if column_name.nil?
      column = table["columns"][column_name]
      ids.concat(extract_ids_from_column(column, processed_targets))
    end
    ids.uniq
  end

  def extract_ids_from_table(table, processed_targets)
    return [] if processed_targets.key?(table)
    processed_targets[table] = true

    ids = []
    ids << table["id"]
    key_type = table["key_type"]
    if key_type && key_type["type"] == "reference"
      ids.concat(extract_ids_from_table(@schema["tables"][key_type["name"]],
                                        processed_targets))
    end
    value_type = table["value_type"]
    if value_type && value_type["type"] == "reference"
      ids.concat(extract_ids_from_table(@schema["tables"][value_type["name"]],
                                        processed_targets))
    end
    ids
  end

  def extract_ids_from_column(column, processed_targets)
    return [] if processed_targets.key?(column)
    processed_targets[column] = true

    ids = []
    ids << column["id"]
    value_type = column["value_type"]
    if value_type && value_type["type"] == "reference"
      ids.concat(extract_ids_from_table(@schema["tables"][value_type["name"]],
                                        processed_targets))
    end
    sources = column["sources"] || []
    sources.each do |source|
      source_table = @schema["tables"][source["table"]]
      ids.concat(extract_ids_from_table(source_table, processed_targets))
      source_name = source["name"]
      source_column = source_table["columns"][source_name]
      ids.concat(extract_ids_from_column(source_column, processed_targets))
    end
    indexes = column["indexes"] || []
    indexes.each do |index|
      index_table = @schema["tables"][index["table"]]
      ids.concat(extract_ids_from_table(index_table, processed_targets))
      index_column = index_table["columns"][index["name"]]
      ids.concat(extract_ids_from_column(index_column, processed_targets))
    end
    ids
  end
end

command = CopyRelatedFiles.new
exit(command.run(ARGV))
