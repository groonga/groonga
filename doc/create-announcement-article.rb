require 'optparse'

class ArticleGenerator
  attr_reader :output_file_path

  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @groonga_version = groonga_version
    @groonga_previous_version = groonga_previous_version
    @groonga_version_in_link = groonga_version.gsub(".", "-")
    @release_date = release_date
    @release_date_in_link = release_date.gsub("-", "/")
  end

  def generate_article
  end

  def extract_latest_release_note
    latest_release_note = File.read(@input_file_path).split(/#{@release_headline_regexp}/)[1]
    latest_release_note.gsub(/^\R\R/, "\n").strip
  end

  def generate
    File.open(@output_file_path, "w") do |file|
      article = generate_article
      file.puts(article)
    end
  end
end

class MarkdownArticleGenerator < ArticleGenerator
  def extract_latest_release_note
    super
  end
end

class MarkdownEnArticleGenerator < MarkdownArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @input_file_path = "./locale/en/markdown/news.md"
    @release_headline_regexp = "## Release.+\\d\\d\\d\\d-\\d\\d-\\d\\d.*"
  end

  def generate_article
    <<-"TEMPLATE"
## Groonga #{@groonga_version} has been released

[Groonga #{@groonga_version}](#{@link_prefix}/news.html#release-#{@groonga_version_in_link}) has been released!

How to install: [Install](#{@link_prefix}/install.html)

### Changes

Here are important changes in this release:

#{extract_latest_release_note}

### Known Issues

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* `*<` and `*>` only valid when we use `query()` the right side of filter condition.

  If we specify as below, `*<` and `*>` work as `&&`.

  * `'content @ "Groonga" *< content @ "Mroonga"'`

* Groonga may not return records that should match caused by `GRN_II_CURSOR_SET_MIN_ENABLE`.

### Conclusion

Please refert to the following news for more details.

[News Release #{@groonga_version}](#{@link_prefix}/news.html#release-#{@groonga_version_in_link})

Let's search by Groonga!
    TEMPLATE
  end
end

class MarkdownJaArticleGenerator < MarkdownArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @input_file_path = "./locale/ja/markdown/news.md"
    @release_headline_regexp = "## .*リリース.+\\d\\d\\d\\d-\\d\\d-\\d\\d.*"
  end

  def generate_article
    <<-"TEMPLATE"
## Groonga #{@groonga_version}リリース

[Groonga #{@groonga_version}](#{@link_prefix}/news.html#release-#{@groonga_version_in_link})をリリースしました！

それぞれの環境毎のインストール方法: [インストール](#{@link_prefix}/install.html)

### 変更内容
    
主な変更点は以下の通りです。

#{extract_latest_release_note}

### 既知の問題

* 現在Groongaには、ベクターカラムに対してデータを大量に追加、削除、更新した際にデータが破損することがある問題があります。

* `*<` と `*>` は、filter条件の右辺に `query()` を使う時のみ有効です。もし、以下のように指定した場合、 `*<` と `*>` は `&&` として機能します。

  ``
  'content @ "Groonga" *< content @ "Mroonga"'
  ``

* `GRN_II_CURSOR_SET_MIN_ENABLE` が原因でマッチするはずのレコードを返さないことがあります。

### さいごに

詳細については、以下のお知らせ、リリース自慢会も参照してください。

[お知らせ #{@groonga_version}リリース](#{@link_prefix}/news.html#release-#{@groonga_version_in_link})

[リリース自慢会](https://www.youtube.com/playlist?list=PLKb0MEIU7gvRxTDecELqAOzOsa21dSwtU)

リリース自慢会は、リリースノートを見ながら、Groongaの新機能・バグ修正を開発者が自慢する会です。

毎月リリースされている最新バージョンについて、開発者的に「これ！」という機能や、「ここをがんばった！」ということを紹介しています。
Liveチャットでコメントも受け付けていますので、気になっていることを質問したり、気になっていたあの機能について聞いたりすることも可能です。

それでは、Groongaでガンガン検索してください！
    TEMPLATE
  end
end

class BlogEnArticleGenerator < MarkdownEnArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "#{groonga_org_repository}/en/_posts/#{release_date}-groonga-#{groonga_version}.md"
    @link_prefix = "/docs"
  end

  def generate_article
    article_base = super
    prefix = <<"PREFIX"
---
layout: post.en
title: Groonga #{@groonga_version} has been released
description: Groonga #{@groonga_version} has been released!
---

PREFIX

    prefix + article_base
  end
end

class BlogJaArticleGenerator < MarkdownJaArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "#{groonga_org_repository}/ja/_posts/#{release_date}-groonga-#{groonga_version}.md"
    @link_prefix = "/ja/docs"
  end

  def generate_article
    article_base = super
    prefix = <<"PREFIX"
---
layout: post.ja
title: Groonga #{@groonga_version}リリース
description: Groonga #{@groonga_version}をリリースしました！
---

PREFIX

    prefix + article_base
  end
end

class DiscussionsEnArticleGenerator < MarkdownEnArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "./tmp/discussions-en-#{release_date}-groonga-#{groonga_version}.md"
    @link_prefix = "https://groonga.org/docs"
  end

  def generate
    Dir.mkdir("./tmp") unless Dir.exist?("./tmp")
    super
  end
end

class DiscussionsJaArticleGenerator < MarkdownJaArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "./tmp/discussions-ja-#{release_date}-groonga-#{groonga_version}.md"
    @link_prefix = "https://groonga.org/ja/docs"
  end

  def generate
    Dir.mkdir("./tmp") unless Dir.exist?("./tmp")
    super
  end
end

class FacebookArticleGenerator < ArticleGenerator
  def generate
    Dir.mkdir("./tmp") unless Dir.exist?("./tmp")
    super
  end
end

class FacebookEnArticleGenerator < FacebookArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @input_file_path = "./locale/en/text/news.txt"
    @output_file_path = "./tmp/facebook-en-#{release_date}-groonga-#{groonga_version}.txt"
    @release_headline_regexp = ".*Release.+\\d\\d\\d\\d-\\d\\d-\\d\\d.*\\n=.+"
  end

  def generate_article
    <<-"TEMPLATE"
Hi,
Groonga #{@groonga_version} has been released!
    
https://groonga.org/docs/news.html#release-#{@groonga_version_in_link}
https://groonga.org/en/blog/#{@release_date_in_link}/groonga-#{@groonga_version_in_link}.html
    
Install: https://groonga.org/docs/install.html
    
Characteristics: https://groonga.org/docs/characteristic.html
    
The topics in this release:

#{extract_latest_release_note}

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* "*<" and "*>" only valid when we use "query()" the right side of filter condition.
  If we specify as below, "*<" and "*>" work as &&.

    'content @ "Groonga" *< content @ "Mroonga"'

* Groonga may not return records that should match caused by "GRN_II_CURSOR_SET_MIN_ENABLE".
    TEMPLATE
  end
end

class FacebookJaArticleGenerator < FacebookArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @input_file_path = "./locale/ja/text/news.txt"
    @output_file_path = "./tmp/facebook-ja-#{release_date}-groonga-#{groonga_version}.txt"
    @release_headline_regexp = ".*リリース.+\\d\\d\\d\\d-\\d\\d-\\d\\d.*\\n=.+"
    @start_headline = "#{groonga_version}"
    @finish_headline = "#{groonga_previous_version}"
  end

  def generate_article
    <<-"TEMPLATE"
Groonga #{@groonga_version} をリリースしました！

    https://groonga.org/ja/docs/news.html#release-#{@groonga_version_in_link}

変更点一覧:

    https://groonga.org/ja/blog/#{@release_date_in_link}/groonga-#{@groonga_version_in_link}.html

変更内容
========

主な変更点は以下の通りです。

#{extract_latest_release_note}

既知の問題
----------

* 現在Groongaには、ベクターカラムに対してデータを大量に追加、削除、更新した際にデータが破損することがある問題があります。

* "*<" と "*>" は、filter条件の右辺に "query()" を使う時のみ有効です。もし、以下のように指定した場合、 "*<" と "*>" は "&&" として機能します。

  'content @ "Groonga" *< content @ "Mroonga"'

* "GRN_II_CURSOR_SET_MIN_ENABLE" が原因でマッチするはずのレコードを返さないことがあります。
    TEMPLATE
  end
end

class TwitterEnArticleBaseGenerator < ArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "./tmp/twitter-en-#{release_date}-groonga-#{groonga_version}-base.txt"
  end

  def generate_article
    "Groonga #{@groonga_version} has been released!(#{@release_date}) " + 
    "https://groonga.org/en/blog/#{@release_date_in_link}/groonga-#{@groonga_version}.html"
  end
end

class TwitterJaArticleBaseGenerator < ArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "./tmp/twitter-ja-#{release_date}-groonga-#{groonga_version}-base.txt"
  end

  def generate_article
    "Groonga #{@groonga_version}をリリースしました！(#{@release_date}) " +
    "https://groonga.org/ja/blog/#{@release_date_in_link}/groonga-#{@groonga_version}.html"
  end
end

option = {}
OptionParser.new do |opt|
  opt.on('--release-date VALUE', "YYYY-MM-DD") { |v| option[:release_date] = v }
  opt.on('--groonga_version VALUE', "e.g. 12.1.1") { |v| option[:groonga_version] = v }
  opt.on('--groonga_previous_version VALUE', "e.g. 12.1.0") { |v| option[:groonga_previous_version] = v }
  opt.on('--groonga_org_repository VALUE', "e.g. $HOME/work/groonga.org") { |v| option[:groonga_org_repository] = v }
  opt.parse!(ARGV)
end

generator_class_list = [
  BlogEnArticleGenerator,
  BlogJaArticleGenerator,
  DiscussionsEnArticleGenerator,
  DiscussionsJaArticleGenerator,
  FacebookEnArticleGenerator,
  FacebookJaArticleGenerator,
  TwitterEnArticleBaseGenerator,
  TwitterJaArticleBaseGenerator
]

generator_class_list.each do |generator_class|
  generator = generator_class.new(option[:release_date],
                                  option[:groonga_version], 
                                  option[:groonga_previous_version],
                                  option[:groonga_org_repository])
  generator.generate
  puts generator.output_file_path
end

