class Time
  def iso8601
    format = "%04d-%02d-%02dT%02d:%02d:%02d.%06d"
    if utc?
      format << "Z"
    end
    usec_workaround = usec
    # https://github.com/mruby/mruby/issues/6059
    usec_workaround = 0 if usec_workaround < 0
    format % [year, month, day, hour, min, sec, usec_workaround]
  end
end
