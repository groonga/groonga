class ArticleGenerator
  attr_reader :output_file_path

  def initialize(release_date, version, previous_version)
    @version = version
    @previous_version = previous_version
    @version_in_link = version.gsub(".", "-")
    @release_date = release_date
    @release_date_in_link = release_date.gsub("-", "/")
  end

  def extract_latest_release_note
    latest_release_note = File.read(@input_file_path).split(/#{@release_headline_regexp_pattern}/)[1]
    latest_release_note.gsub(/^\R\R/, "\n").strip
  end

  def generate_article
    extract_latest_release_note
  end

  def generate
    File.open(@output_file_path, "w") do |file|
      article = generate_article
      file.puts(article)
    end
  end
end