$trace = true

MRuby::Lockfile.disable if MRuby.const_defined?(:Lockfile)

MRuby::Build.new do |conf|
  if ENV["MRUBY_VC"] || ENV["VisualStudioVersion"] || ENV["VSINSTALLDIR"]
    conf.toolchain :visualcpp
  else
    conf.toolchain :gcc
  end

  oniguruma_include_path = ENV["MRUBY_ONIGURUMA_INCLUDE_PATH"]
  if oniguruma_include_path
    conf.cc.include_paths << oniguruma_include_path
  end

  conf.enable_debug

  conf.gem :core => "mruby-array-ext"
  conf.gem :core => "mruby-compiler"
  conf.gem :core => "mruby-enum-ext"
  conf.gem :core => "mruby-enum-lazy"
  conf.gem :core => "mruby-enumerator"
  conf.gem :core => "mruby-fiber"
  conf.gem :core => "mruby-hash-ext"
  conf.gem :core => "mruby-io"
  conf.gem :core => "mruby-kernel-ext"
  conf.gem :core => "mruby-math"
  conf.gem :core => "mruby-metaprog"
  conf.gem :core => "mruby-numeric-ext"
  conf.gem :core => "mruby-object-ext"
  conf.gem :core => "mruby-objectspace"
  conf.gem :core => "mruby-print"
  conf.gem :core => "mruby-proc-ext"
  conf.gem :core => "mruby-random"
  conf.gem :core => "mruby-range-ext"
  conf.gem :core => "mruby-sprintf"
  conf.gem :core => "mruby-string-ext"
  conf.gem :core => "mruby-struct"
  conf.gem :core => "mruby-symbol-ext"
  conf.gem :core => "mruby-time"
  conf.gem :core => "mruby-toplevel-ext"

  conf.gem "mrbgems/mruby-dir"
  conf.gem "mrbgems/mruby-env"
  conf.gem "mrbgems/mruby-errno"
  conf.gem "mrbgems/mruby-pp"
  conf.gem "mrbgems/mruby-slop"
  conf.gem "mrbgems/mruby-tsort"
  conf.gem "mrbgems/mruby-file-stat"
  conf.gem "mrbgems/mruby-onig-regexp"
end
