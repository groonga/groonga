#!/usr/bin/ruby

require 'groonga'

if ARGV.empty?
  $db = Groonga::Ctx::connect
else
  $db = Groonga::Ctx::open(ARGV[0])
end

$db.send("'(a ? b)")
loop {
  r = $db.recv
  p r
  break if r[2] == 0
}

$db.send("hoge hoge")
loop {
  r = $db.recv
  p r
  break if r[2] == 0
}
