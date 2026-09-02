#!/usr/bin/env ruby

if ARGV.size != 2
  puts("Usage: #{$0} BUILD_COFNIG.RB MRUBY_SOURCE_DIR")
  exit(false)
end

require "find"

build_config_rb = ARGV.shift
mruby_source_dir = ARGV.shift

module MRuby
  class Build
    class << self
      def source_dir=(dir)
        @@source_dir = dir
      end

      def latest
        @@latest
      end
    end

    attr_reader :config
    def initialize(&block)
      @@latest = self
      @config = Config.new(@@source_dir)
      @config.instance_eval(&block)
    end
  end

  class Config
    attr_reader :gem_dirs
    def initialize(source_dir)
      @source_dir = source_dir
      @gem_dirs = []
    end

    def toolchain(*args)
      # ignore
    end

    def enable_debug
      # ignore
    end

    def gem(gem_dir)
      if gem_dir.is_a?(Hash)
        gem_dir = load_special_path_gem(gem_dir)
        return if gem_dir.nil?
      end
      @gem_dirs << gem_dir
    end

    private
    def load_special_path_gem(params)
      if params[:core]
        "#{@source_dir}/mrbgems/#{params[:core]}"
      elsif params[:github]
        nil
      else
        raise "Unsupported gem options: #{params.inspect}"
      end
    end
  end
end

MRuby::Build.source_dir = mruby_source_dir
load build_config_rb
build = MRuby::Build.latest

sources = []
source_dirs = ["#{mruby_source_dir}/src"] + build.config.gem_dirs
source_dirs.each do |source_dir|
  Find.find(source_dir) do |path|
    if File.directory?(path)
      # Ignore mruby bundled in a gem to avoid duplicated entries.
      Find.prune if path == "#{source_dir}/mruby"
      # Use only the POSIX port. CMakeLists.txt and Makefile.am
      # substitute the Windows port on Windows.
      Find.prune if path == "#{source_dir}/ports/win"
      if File.dirname(path) == "#{source_dir}/lib/prism"
        # Prism has many non-C bindings and tools. Only src/ and
        # include/ are needed to build the mruby compiler.
        case File.basename(path)
        when "include", "src"
        else
          Find.prune
        end
      end
    end
    case path
    when /\/test\//
      next
    when "#{source_dir}/lib/prism/include/prism/ast.h",
         "#{source_dir}/lib/prism/include/prism/diagnostic.h",
         "#{source_dir}/lib/prism/src/diagnostic.c",
         "#{source_dir}/lib/prism/src/node.c",
         "#{source_dir}/lib/prism/src/prettyprint.c",
         "#{source_dir}/lib/prism/src/serialize.c",
         "#{source_dir}/lib/prism/src/token_type.c"
      # Ignore generated Prism sources.
      next
    when /\.[ch]\z/
      sources << path
    end
  end
end

indented_sources = sources.collect do |source|
  "\t#{source}"
end
puts("libmruby_la_SOURCES = \\")
puts(indented_sources.join(" \\\n"))
