#!/usr/bin/env ruby

groonga_org_repository_path = ENV['GROONGA_ORG_PATH']
if groonga_org_repository_path.nil?
  puts("Usage: GROONGA_ORG_PATH=xxx ruby generate-blog-entry-from-release-note.rb")
  return
end

base_version_file = File.join(__dir__,
                              "..",
                              "base_version")
groonga_version = File.read(base_version_file).chomp

def contents_ja(groonga_version)
  groonga_major_version = groonga_version.split('.')[0];

  contents = <<CONTENTS
---
layout: post.ja
title: Groonga #{groonga_version}リリース
description: Groonga #{groonga_version}をリリースしました！
---

## Groonga #{groonga_version}リリース

Groonga #{groonga_version}をリリースしました！

それぞれの環境毎のインストール方法: [インストール](/ja/docs/install.html)

### 変更内容

主な変更点は以下のリンクを参照してください。

[Groonga #{groonga_version} release-note](/ja/docs/news/#{groonga_major_version}.html#release-#{groonga_version})

CONTENTS
end

def contents_en(groonga_version)
  groonga_major_version = groonga_version.split('.')[0];

  contents = <<CONTENTS
---
layout: post.en
title: Groonga #{groonga_version} has been released
description: Groonga #{groonga_version} has been released!
---

## Groonga #{groonga_version} has been released

Groonga #{groonga_version} has been released!

How to install: [Install](/docs/install.html)

### Changes

Please refer the following link.

[Groonga #{groonga_version} release-note](/docs/news/#{groonga_major_version}.html#release-#{groonga_version})

CONTENTS
end

groonga_blog_file_name = "#{Time.now.strftime("%F")}-groonga-#{groonga_version}.md"
File.open("#{groonga_org_repository_path}/ja/_posts/#{groonga_blog_file_name}", "w") do |blog_ja|
  blog_ja.write(contents_ja(groonga_version))
end

File.open("#{groonga_org_repository_path}/en/_posts/#{groonga_blog_file_name}", "w") do |blog_en|
  blog_en.write(contents_en(groonga_version))
end
