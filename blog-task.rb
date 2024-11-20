# Copyright(C) 2024  Horimoto Yasuhiro <horimoto@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

class BlogTask
  include Rake::DSL

  def initialize(package)
    @package = package
    @version = detect_version
  end

  def define
    define_blog_task
  end

  private
  def env_var(name, default=nil)
    value = ENV[name] || default
    raise "${#{name}} is missing" if value.nil?
    value
  end

  def document_repository_path
    raise NotImplementedError
  end

  def detect_version
    raise NotImplementedError
  end

  def define_blog_task
    namespace :document do
      namespace :blog do
        desc "Generate Blog from release note"
        task :generate do
          blog_file_name = "#{Time.now.strftime("%F")}-#{package}-#{version}.md"
          File.open("#{document_repository_path}/ja/_posts/#{blog_file_name}", "w") do |blog_ja|
            blog_ja.write(contents_ja(@package.capitalize, @version))
          end

          File.open("#{document_repository_path}/en/_posts/#{blog_file_name}", "w") do |blog_en|
            blog_en.write(contents_en(@package.capitalize, @version))
          end
        end
      end
    end
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
end
