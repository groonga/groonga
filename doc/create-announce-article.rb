release_date = "2023-01-07"
groonga_previous_version = "12.1.0"
groonga_version = "12.1.1"
groonga_org_repository = "/home/t-hashida/gitdir/groonga.org"

class ArticleGenerator
  attr_reader :output_file_path

  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @groonga_version = groonga_version
    @groonga_previous_version = groonga_previous_version
    @groonga_version_in_link = groonga_version.gsub(".", "-")
  end

  def get_article
    template = get_template
    changes = get_changes
    template % {groonga_version:@groonga_version, groonga_version_in_link:@groonga_version_in_link, changes: changes }
  end

  def enumerate_changes
    latest_release = false
    File.open(@input_file_path, "r") do |file|
      file.each_line do |line|
        if not latest_release and line.include?(@start_headline)
          latest_release = true
          next
        end
        break if latest_release and line.include?(@finish_headline)
        yield line if latest_release
      end
    end
  end

  def get_changes
    changes = ""
    enumerate_changes do |line|
      changes << line
    end
    changes.gsub(/^\R\R/, "\n").strip
  end

  def output_file
    File.open(@output_file_path, "w") do |file|
      article = get_article
      file.puts(article)
    end
  end
end

class EnArticleGenerator < ArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @input_file_path = "./locale/en/markdown/news.md"
    @start_headline = "## Release #{groonga_version}"
    @finish_headline = "## Release #{groonga_previous_version}"
  end
end

class JaArticleGenerator < ArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @input_file_path = "./locale/ja/markdown/news.md"
    @start_headline = "## #{groonga_version}"
    @finish_headline = "## #{groonga_previous_version}"
  end
end

class BlogEnArticleGenerator < EnArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "#{groonga_org_repository}/en/_posts/#{release_date}-groonga-#{groonga_version}.md"
  end

  def get_changes
    changes = super
    changes.gsub(/(\[.+?\])\((.+?).md(.*?\))/, "\\1(/docs/\\2.html\\3")
  end

  def get_template
    <<-"TEMPLATE"
---
layout: post.en
title: Groonga %<groonga_version>s has been released
description: Groonga %<groonga_version>s has been released!
---

## Groonga %<groonga_version>s has been released

[Groonga %<groonga_version>s](/docs/news.html#release-%<groonga_version_in_link>s) has been released!

How to install: [Install](/docs/install.html)

### Changes

Here are important changes in this release:

%<changes>s

### Known Issues

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* `*<` and `*>` only valid when we use `query()` the right side of filter condition.

  If we specify as below, `*<` and `*>` work as `&&`.

  * `'content @ "Groonga" *< content @ "Mroonga"'`

* Groonga may not return records that should match caused by `GRN_II_CURSOR_SET_MIN_ENABLE`.

### Conclusion

Please refert to the following news for more details.

[News Release %<groonga_version>s](/docs/news.html#release-%<groonga_version_in_link>s)

Let's search by Groonga!
    TEMPLATE
  end
end

class BlogJaArticleGenerator < JaArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "#{groonga_org_repository}/ja/_posts/#{release_date}-groonga-#{groonga_version}.md"
  end

  def get_changes
    changes = super
    changes.gsub(/(\[.+?\])\((.+?).md(.*?\))/, "\\1(/ja/docs/\\2.html\\3")
  end

  def get_template
    <<-"TEMPLATE"
---
layout: post.ja
title: Groonga %<groonga_version>sリリース
description: Groonga %<groonga_version>sをリリースしました！
---

## Groonga %<groonga_version>sリリース

[Groonga %<groonga_version>s](/ja/docs/news.html#release-%<groonga_version_in_link>s)をリリースしました！

それぞれの環境毎のインストール方法: [インストール](/ja/docs/install.html)

### 変更内容
    
主な変更点は以下の通りです。

%<changes>s

### 既知の問題

* 現在Groongaには、ベクターカラムに対してデータを大量に追加、削除、更新した際にデータが破損することがある問題があります。

* `*<` と `*>` は、filter条件の右辺に `query()` を使う時のみ有効です。もし、以下のように指定した場合、 `*<` と `*>` は `&&` として機能します。

  ``
  'content @ "Groonga" *< content @ "Mroonga"'
  ``

* `GRN_II_CURSOR_SET_MIN_ENABLE` が原因でマッチするはずのレコードを返さないことがあります。

### さいごに

詳細については、以下のお知らせ、リリース自慢会も参照してください。

[お知らせ %<groonga_version>sリリース](/ja/docs/news.html#release-%<groonga_version_in_link>s)

[リリース自慢会](https://www.youtube.com/playlist?list=PLKb0MEIU7gvRxTDecELqAOzOsa21dSwtU)

リリース自慢会は、リリースノートを見ながら、Groongaの新機能・バグ修正を開発者が自慢する会です。

毎月リリースされている最新バージョンについて、開発者的に「これ！」という機能や、「ここをがんばった！」ということを紹介しています。
Liveチャットでコメントも受け付けていますので、気になっていることを質問したり、気になっていたあの機能について聞いたりすることも可能です。

それでは、Groongaでガンガン検索してください！
    TEMPLATE
  end
end

class DiscussionsEnArticleGenerator < EnArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "./tmp/discussions-en-#{release_date}-groonga-#{groonga_version}.md"
  end

  def get_changes
    changes = super
    changes.gsub(/(\[.+?\])\((.+?).md(.*?\))/, "\\1(https://groonga.org/docs/\\2.html\\3")
  end

  def output_file
    Dir.mkdir("./tmp") unless Dir.exist?("./tmp")
    super
  end

  def get_template
    <<-"TEMPLATE"
## Groonga %<groonga_version>s has been released

[Groonga %<groonga_version>s](https://groonga.org/docs/news.html#release-%<groonga_version_in_link>s) has been released!

How to install: [Install](https://groonga.org/docs/install.html)

### Changes

Here are important changes in this release:

%<changes>s

### Known Issues

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* `*<` and `*>` only valid when we use `query()` the right side of filter condition.

  If we specify as below, `*<` and `*>` work as `&&`.

  * `'content @ "Groonga" *< content @ "Mroonga"'`

* Groonga may not return records that should match caused by `GRN_II_CURSOR_SET_MIN_ENABLE`.

### Conclusion

Please refert to the following news for more details.

[News Release %<groonga_version>s](https://groonga.org/docs/news.html#release-%<groonga_version_in_link>s)

Let's search by Groonga!
    TEMPLATE
  end
end

class DiscussionsJaArticleGenerator < JaArticleGenerator
  def initialize(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    super(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
    @output_file_path = "./tmp/discussions-ja-#{release_date}-groonga-#{groonga_version}.md"
  end

  def get_changes
    changes = super
    changes.gsub(/(\[.+?\])\((.+?).md(.*?\))/, "\\1(https://groonga.org/ja/docs/\\2.html\\3")
  end

  def output_file
    Dir.mkdir("./tmp") unless Dir.exist?("./tmp")
    super
  end

  def get_template
    <<-"TEMPLATE"
## Groonga %<groonga_version>sリリース

[Groonga %<groonga_version>s](https://groonga.org/ja/docs/news.html#release-%<groonga_version_in_link>s)をリリースしました！

それぞれの環境毎のインストール方法: [インストール](https://groonga.org/ja/docs/install.html)

### 変更内容
    
主な変更点は以下の通りです。

%<changes>s

### 既知の問題

* 現在Groongaには、ベクターカラムに対してデータを大量に追加、削除、更新した際にデータが破損することがある問題があります。

* `*<` と `*>` は、filter条件の右辺に `query()` を使う時のみ有効です。もし、以下のように指定した場合、 `*<` と `*>` は `&&` として機能します。

  ``
  'content @ "Groonga" *< content @ "Mroonga"'
  ``

* `GRN_II_CURSOR_SET_MIN_ENABLE` が原因でマッチするはずのレコードを返さないことがあります。

### さいごに

詳細については、以下のお知らせ、リリース自慢会も参照してください。

[お知らせ %<groonga_version>sリリース](https://groonga.org/ja/docs/news.html#release-%<groonga_version_in_link>s)

[リリース自慢会](https://www.youtube.com/playlist?list=PLKb0MEIU7gvRxTDecELqAOzOsa21dSwtU)

リリース自慢会は、リリースノートを見ながら、Groongaの新機能・バグ修正を開発者が自慢する会です。

毎月リリースされている最新バージョンについて、開発者的に「これ！」という機能や、「ここをがんばった！」ということを紹介しています。
Liveチャットでコメントも受け付けていますので、気になっていることを質問したり、気になっていたあの機能について聞いたりすることも可能です。

それでは、Groongaでガンガン検索してください！
    TEMPLATE
  end
end

generator_list = [
  BlogEnArticleGenerator.new(release_date, groonga_version, groonga_previous_version, groonga_org_repository),
  BlogJaArticleGenerator.new(release_date, groonga_version, groonga_previous_version, groonga_org_repository),
  DiscussionsEnArticleGenerator.new(release_date, groonga_version, groonga_previous_version, groonga_org_repository),
  DiscussionsJaArticleGenerator.new(release_date, groonga_version, groonga_previous_version, groonga_org_repository)
]

generator_list.each do |generator|
  generator.output_file
  puts generator.output_file_path
end

