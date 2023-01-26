require 'optparse'
require 'fileutils'
require './announcement-article-generator'

class GroongaArticleGenerator < ArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version)
  end
end

class MarkdownEnArticleGenerator < GroongaArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @input_file_path = "./locale/en/markdown/news.md"
    @release_headline_regexp_pattern = "## Release.+\\d\\d\\d\\d-\\d\\d-\\d\\d.*"
  end

  def generate_article
    <<-ARTICLE
## Groonga #{@version} has been released

[Groonga #{@version}](#{@link_prefix}/news.html#release-#{@version_in_link}) has been released!

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

[News Release #{@version}](#{@link_prefix}/news.html#release-#{@version_in_link})

Let's search by Groonga!
    ARTICLE
  end
end

class MarkdownJaArticleGenerator < GroongaArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @input_file_path = "./locale/ja/markdown/news.md"
    @release_headline_regexp_pattern = "## .*リリース.+\\d\\d\\d\\d-\\d\\d-\\d\\d.*"
  end

  def generate_article
    <<-ARTICLE
## Groonga #{@version}リリース

[Groonga #{@version}](#{@link_prefix}/news.html#release-#{@version_in_link})をリリースしました！

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

[お知らせ #{@version}リリース](#{@link_prefix}/news.html#release-#{@version_in_link})

[リリース自慢会](https://www.youtube.com/playlist?list=PLKb0MEIU7gvRxTDecELqAOzOsa21dSwtU)

リリース自慢会は、リリースノートを見ながら、Groongaの新機能・バグ修正を開発者が自慢する会です。

毎月リリースされている最新バージョンについて、開発者的に「これ！」という機能や、「ここをがんばった！」ということを紹介しています。
Liveチャットでコメントも受け付けていますので、気になっていることを質問したり、気になっていたあの機能について聞いたりすることも可能です。

それでは、Groongaでガンガン検索してください！
    ARTICLE
  end
end

class BlogEnArticleGenerator < MarkdownEnArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @output_file_path = "#{groonga_org_repository}/en/_posts/#{release_date}-groonga-#{version}.md"
    @link_prefix = "/docs"
  end

  def generate_article
    article_base = super
    prefix = <<-ARTICLE
---
layout: post.en
title: Groonga #{@version} has been released
description: Groonga #{@version} has been released!
---

#{super}
    ARTICLE
  end
end

class BlogJaArticleGenerator < MarkdownJaArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @output_file_path = "#{groonga_org_repository}/ja/_posts/#{release_date}-groonga-#{version}.md"
    @link_prefix = "/ja/docs"
  end

  def generate_article
    article_base = super
    prefix = <<-ARTICLE
---
layout: post.ja
title: Groonga #{@version}リリース
description: Groonga #{@version}をリリースしました！
---

#{super}
    ARTICLE
  end
end

class DiscussionsEnArticleGenerator < MarkdownEnArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @output_file_path = "./tmp/discussions-en-#{release_date}-groonga-#{version}.md"
    @link_prefix = "https://groonga.org/docs"
  end

  def generate
    FileUtils.mkdir_p("tmp")
    super
  end
end

class DiscussionsJaArticleGenerator < MarkdownJaArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @output_file_path = "./tmp/discussions-ja-#{release_date}-groonga-#{version}.md"
    @link_prefix = "https://groonga.org/ja/docs"
  end

  def generate
    FileUtils.mkdir_p("tmp")
    super
  end
end

class FacebookArticleGenerator < GroongaArticleGenerator
  def generate
    FileUtils.mkdir_p("tmp")
    super
  end
end

class FacebookEnArticleGenerator < FacebookArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @input_file_path = "./locale/en/text/news.txt"
    @output_file_path = "./tmp/facebook-en-#{release_date}-groonga-#{version}.txt"
    @release_headline_regexp_pattern = ".*Release.+\\d\\d\\d\\d-\\d\\d-\\d\\d.*\\n=.+"
  end

  def generate_article
    <<-ARTICLE
Hi,
Groonga #{@version} has been released!
    
https://groonga.org/docs/news.html#release-#{@version_in_link}
https://groonga.org/en/blog/#{@release_date_in_link}/groonga-#{@version_in_link}.html
    
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
    ARTICLE
  end
end

class FacebookJaArticleGenerator < FacebookArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @input_file_path = "./locale/ja/text/news.txt"
    @output_file_path = "./tmp/facebook-ja-#{release_date}-groonga-#{version}.txt"
    @release_headline_regexp_pattern = ".*リリース.+\\d\\d\\d\\d-\\d\\d-\\d\\d.*\\n=.+"

  end

  def generate_article
    <<-ARTICLE
Groonga #{@version} をリリースしました！

    https://groonga.org/ja/docs/news.html#release-#{@version_in_link}

変更点一覧:

    https://groonga.org/ja/blog/#{@release_date_in_link}/groonga-#{@version_in_link}.html

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
    ARTICLE
  end
end

class TwitterEnArticleBaseGenerator < GroongaArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @output_file_path = "./tmp/twitter-en-#{release_date}-groonga-#{version}-base.txt"
  end

  def generate_article
    "Groonga #{@version} has been released!(#{@release_date}) " + 
    "https://groonga.org/en/blog/#{@release_date_in_link}/groonga-#{@version}.html"
  end
end

class TwitterJaArticleBaseGenerator < GroongaArticleGenerator
  def initialize(release_date, version, previous_version, groonga_org_repository)
    super(release_date, version, previous_version, groonga_org_repository)
    @output_file_path = "./tmp/twitter-ja-#{release_date}-groonga-#{version}-base.txt"
  end

  def generate_article
    "Groonga #{@version}をリリースしました！(#{@release_date}) " +
    "https://groonga.org/ja/blog/#{@release_date_in_link}/groonga-#{@version}.html"
  end
end

option = {}
OptionParser.new do |opt|
  opt.on('--release-date=DATE', "YYYY-MM-DD") { |v| option[:release_date] = v }
  opt.on('--version=VERSION', "e.g. 12.1.1") { |v| option[:version] = v }
  opt.on('--previous-version=VERSION', "e.g. 12.1.0") { |v| option[:previous_version] = v }
  opt.on('--groonga-org-repository=PATH', "e.g. $HOME/work/groonga.org") { |v| option[:groonga_org_repository] = v }
  opt.parse!(ARGV)
end

generator_classes = [
  BlogEnArticleGenerator,
  BlogJaArticleGenerator,
  DiscussionsEnArticleGenerator,
  DiscussionsJaArticleGenerator,
  FacebookEnArticleGenerator,
  FacebookJaArticleGenerator,
  TwitterEnArticleBaseGenerator,
  TwitterJaArticleBaseGenerator
]

generator_classes.each do |generator_class|
  generator = generator_class.new(option[:release_date],
                                  option[:version], 
                                  option[:previous_version],
                                  option[:groonga_org_repository])
  generator.generate
  puts generator.output_file_path
end

