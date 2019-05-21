#@on-error omit
plugin_register ruby/eval
#@on-error default

ruby_eval "(Groonga::Logger::Flags::TIME | :location).to_s"
