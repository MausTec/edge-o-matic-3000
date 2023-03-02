#!/usr/bin/env ruby
#
# Config is pretty spread out right now, so this lints it up.

DIR = File.expand_path(File.join(File.dirname(__FILE__), ".."))
puts "IN: #{DIR}"

require 'optimist'

opts = Optimist::options do
  banner <<-TEXT
Validate that the Config struct is in sync with documentation and various getters/setters.
TEXT

  opt :fix, "Automatically try and fix stuff", default: false
end

# Parse in Config
config_h = File.join(DIR, "include", "config.h")
in_struct = false

$config_values = []

$errors = []
def error(file, line, message, fixed)
  $errors.push({ fixed: false, file: file, line: (line || -1) + 1, message: message })
end

#== Check Readme
readme_md = File.join(DIR, "README.md")
readme_md_out = []
in_configuration = false

File.foreach(readme_md).with_index do |line, i|
  if line =~ /^## Configuration$/
    in_configuration = true;
  elsif line =~ /^#/
    in_configuration = false;
  end

  if opts[:fix]
    readme_md_out << line
  end

  next unless in_configuration
  match = line =~ /\|(`?\w+`?)\|(.*?)\|(.*?)\|(.*?)\|/
  next unless match
  config = { key: $1, type: $2, default: $3, note: $4 }
  next if $1 == "---" or $1 == "Key"
  $config_values << config;

  unless config[:key] =~ /^`\w+`$/
    error readme_md, i, "Missing backticks around value name: `#{config[:key]}`", true
    if opts[:fix]
      readme_md_out.pop
      readme_md_out << "|`#{config[:key]}`|#{config[:type]}|#{config[:default]}|#{config[:note]}|"
    end
  end

  config[:key].gsub!('`', '');
  puts config
end



###

$config_values.each do |cfg|
  puts "#define #{cfg[:key].upcase}_HELP _HELPSTR(\"#{cfg[:note]}\")"
end

###

puts
# error count
if $errors.length > 0
  $errors.each do |err|
    puts "IN #{err[:file]}:#{err[:line] || 0}\n  > #{err[:message]}"
  end
  puts
  puts "#{$errors.length} errors!"
  exit 1
else
  puts "No errors!"
end