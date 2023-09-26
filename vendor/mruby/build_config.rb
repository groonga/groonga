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

  conf.gem :github => "iij/mruby-dir",
           :checksum_hash => "14bc5c3e51eac16ebc9075b7b62132a0cf5ae724"
  conf.gem :github => "iij/mruby-env",
           :checksum_hash => "056ae324451ef16a50c7887e117f0ea30921b71b"
  conf.gem :github => "iij/mruby-errno",
           :checksum_hash => "b4415207ff6ea62360619c89a1cff83259dc4db0"
  conf.gem :github => "kou/mruby-pp",
           :checksum_hash => "4deddeeef566bbe076391d37dba9986631c2457e"
  conf.gem :github => "kou/mruby-slop",
           :checksum_hash => "38f4795712c8f92e4c9412b5fc81646b3972b12a"
  conf.gem :github => "kou/mruby-tsort",
           :checksum_hash => "6d7f5a56ac7a90847f84186ce1dbc780e41928dc"
  conf.gem :github => "ksss/mruby-file-stat",
           :checksum_hash => "f3e858f01361b9b4a8e77ada52470068630c9530"
  conf.gem :github => "mattn/mruby-onig-regexp",
           :checksum_hash => "2ae5ec5cde0b54fc9c3f81a8cf81f7dac574cff6"
end
