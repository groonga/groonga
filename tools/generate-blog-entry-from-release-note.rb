#!/usr/bin/env ruby

document_repository_path = ARGV[0]
if document_repository_path.nil?
  return
end

package = ARGV[1]
if package.nil?
  return
end

version = ARGV[2]
if version.nil?
  return
end


def contents_ja(package, version)
  major_version = version.split('.')[0];

  contents = <<CONTENTS
---
layout: post.ja
title: #{package} #{version}リリース
description: #{package} #{version}をリリースしました！
---

## #{package} #{version}リリース

#{package} #{version}をリリースしました！

それぞれの環境毎のインストール方法: [インストール](/ja/docs/install.html)

### 変更内容

主な変更点は以下のリンクを参照してください。

[#{package} #{version} release-note](/ja/docs/news/#{major_version}.html#release-#{version.gsub(".", "-")})

CONTENTS
end

def contents_en(package, version)
  major_version = version.split('.')[0];

  contents = <<CONTENTS
---
layout: post.en
title: #{package} #{version} has been released
description: #{package} #{version} has been released!
---

## #{package} #{version} has been released

#{package} #{version} has been released!

How to install: [Install](/docs/install.html)

### Changes

Please refer the following link.

[#{package} #{version} release-note](/docs/news/#{major_version}.html#release-#{version.gsub(".", "-")})

CONTENTS
end

blog_file_name = "#{Time.now.strftime("%F")}-#{package}-#{version}.md"
File.open("#{document_repository_path}/ja/_posts/#{blog_file_name}", "w") do |blog_ja|
  blog_ja.write(contents_ja(package.capitalize, version))
end

File.open("#{document_repository_path}/en/_posts/#{blog_file_name}", "w") do |blog_en|
  blog_en.write(contents_en(package.capitalize, version))
end
