class Time
  def iso8601
    format = "%04d-%02d-%02dT%02d:%02d:%02d.%06d"
    if utc?
      format << "Z"
    end
    format % [year, month, day, hour, min, sec, usec]
  end
end
