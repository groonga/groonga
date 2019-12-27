MRuby::Build.new do |conf|
  if ENV['VisualStudioVersion'] || ENV['VSINSTALLDIR']
    toolchain :visualcpp
  else
    toolchain :gcc
  end

  oniguruma_include_path = ENV["MRUBY_ONIGURUMA_INCLUDE_PATH"]
  if oniguruma_include_path
    conf.cc.include_paths << oniguruma_include_path
  end

  enable_debug

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

  conf.gem :github => "mattn/mruby-onig-regexp",
           :checksum_hash => "0af2124b095474bd2897021df669e61eac4743ec"
  conf.gem :github => "iij/mruby-env",
           :checksum_hash => "056ae324451ef16a50c7887e117f0ea30921b71b"
  conf.gem :github => "kou/mruby-pp",
           :checksum_hash => "ddda20ca273ba532f2025d4ff7ddc8bb223ad8c2"
  conf.gem :github => "kou/mruby-slop",
           :checksum_hash => "0aa5b832315ccd1ddc55f5391f2d8a1e9d6145b0"
  conf.gem :github => "ksss/mruby-file-stat",
           :checksum_hash => "12871584f2e5e2d24f5c54325d3ba3338414e2a4"
  conf.gem :github => "kou/mruby-tsort",
           :checksum_hash => "6d7f5a56ac7a90847f84186ce1dbc780e41928dc"
  conf.gem :github => "iij/mruby-dir",
           :checksum_hash => "14bc5c3e51eac16ebc9075b7b62132a0cf5ae724"
  conf.gem :github => "scalone/mruby-miniz",
           :checksum_hash => "1e839c5810b808937eef39f7b5f981108b2f5af8"
end
