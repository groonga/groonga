$trace = true

MRuby::Lockfile.disable if MRuby.const_defined?(:Lockfile)

MRuby::Build.new do |conf|
  if ENV["MRUBY_VC"] || ENV["VisualStudioVersion"] || ENV["VSINSTALLDIR"]
    conf.toolchain :visualcpp

    # We don't want to fix bundled mruby. So we suppress warnings for
    # mruby. See also vendor/mruby/CMakeLists.txt.

    # 'token' : signed/unsigned mismatch
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4018
    conf.cc.flags << "/wd4018"
    # unary minus operator applied to unsigned type, result still unsigned
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4146
    conf.cc.flags << "/wd4146"
    # 'argument' : conversion from 'type1' to 'type2', possible loss of data
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4244
    conf.cc.flags << "/wd4244"
    # 'var' : conversion from 'size_t' to 'type', possible loss of data
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267
    conf.cc.flags << "/wd4267"
    # Your code uses a function, class member, variable, or typedef
    # that's marked deprecated.
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996
    conf.cc.flags << "/wd4996"
    # This is a bug of Visual Studio: https://developercommunity.visualstudio.com/t/warning-C5287:-operands-are-different-e/10877942
    #
    # Visual Studio 2026 (18.0) fixed this.
    if ENV["VisualStudioVersion"].to_f < 18.0
      # operands are different enum types 'type1' and 'type2'; use an
      # explicit cast to silence this warning
      # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warnings-c5200-through-c5399
      conf.cc.flags << "/wd5287"
    end
  else
    conf.toolchain :gcc
  end

  oniguruma_include_path = ENV["MRUBY_ONIGURUMA_INCLUDE_PATH"]
  if oniguruma_include_path
    conf.cc.include_paths << oniguruma_include_path
  end

  conf.enable_debug

  conf.gem core: "mruby-array-ext"
  conf.gem core: "mruby-compiler"
  conf.gem core: "mruby-dir"
  conf.gem core: "mruby-enum-ext"
  conf.gem core: "mruby-enum-lazy"
  conf.gem core: "mruby-enumerator"
  conf.gem core: "mruby-env"
  conf.gem core: "mruby-errno"
  conf.gem core: "mruby-fiber"
  conf.gem core: "mruby-hash-ext"
  conf.gem core: "mruby-io"
  conf.gem core: "mruby-kernel-ext"
  conf.gem core: "mruby-math"
  conf.gem core: "mruby-metaprog"
  conf.gem core: "mruby-numeric-ext"
  conf.gem core: "mruby-object-ext"
  conf.gem core: "mruby-objectspace"
  conf.gem core: "mruby-proc-ext"
  conf.gem core: "mruby-random"
  conf.gem core: "mruby-range-ext"
  conf.gem core: "mruby-sprintf"
  conf.gem core: "mruby-string-ext"
  conf.gem core: "mruby-struct"
  conf.gem core: "mruby-symbol-ext"
  conf.gem core: "mruby-time"
  conf.gem core: "mruby-toplevel-ext"

  conf.gem "mrbgems/mruby-pp"
  conf.gem "mrbgems/mruby-slop"
  conf.gem "mrbgems/mruby-tsort"
  conf.gem "mrbgems/mruby-file-stat"
  conf.gem "mrbgems/mruby-onig-regexp"
end
