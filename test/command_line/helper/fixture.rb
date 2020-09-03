require "rubygems/package"
require "zlib"

module Fixture
  private
  # tar.gz must use the following tree:
  #
  #   db
  #   db.0000000
  #   db.001
  #   ...
  #
  # For example,
  # test/command_line/fixture/db-with-duplicated-key.tar.gz must be
  # created by the following:
  #
  #   $ rm -rf db*
  #   $ tools/create-hash-table-with-duplicated-key
  #   $ tar cfz test/command_line/fixture/db-with-duplicated-key.tar.gz db*
  def extract_database_tar_gz(tar_gz_path, database_path)
    Zlib::GzipReader.open(tar_gz_path) do |tar_io|
      Gem::Package::TarReader.new(tar_io) do |tar|
        metadata = nil
        tar.each do |entry|
          path = entry.full_name
          path = metadata["path"] || path if metadata
          path = path.gsub(/\Adb/) {database_path.to_s}
          if entry.directory?
            FileUtils.mkdir_p(path)
          elsif entry.file?
            File.open(path, "wb") do |file|
              file.print(entry.read)
            end
          elsif entry.header.typeflag == "x"
            metadata = {}
            entry.read.each_line do |line|
              _size, key_value = line.chomp.split(" ", 2)
              key, value = key_value.split("=", 2)
              metadata[key] = value
            end
            next
          end
          metadata = nil
        end
      end
    end
  end

  def use_fixture(name)
    tar_gz = File.join(__dir__, "..", "fixture", "#{name}.tar.gz")
    extract_database_tar_gz(tar_gz, @database_path.to_s)
  end
end
