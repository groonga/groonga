#@on-error omit
plugin_register ruby/eval
#@on-error default

ruby_eval "(Groonga::Logger::Flags.new([:time, :thread_id]) & [:location, :time]).to_s"
