#!/usr/bin/env ruby
#
# Config is pretty spread out right now, so this lints it up.

DIR = File.expand_path(File.join(File.dirname(__FILE__), ".."))
puts "IN: #{DIR}"

# Parse in Config
config_h = File.join(DIR, "config.h")
in_struct = false

config_values = {}

File.foreach(config_h) do |line|
  if line =~ /struct\s+ConfigStruct/
    in_struct = true
    next
  end

  if !in_struct || line =~ /^\s*\/\// || line =~ /^\s*$/
    next
  end

  if line =~ /\s*}\s*extern/
    in_struct = false
    break
  end

  line =~ /\s*(\w+\s*[*&]?)\s*(\w+)(\[\d*\])?/
  option = {
    type: ($1.to_s + $3.to_s),
    name: $2
  }

  option[:type].gsub!(/\s/, '')
  config_values[option[:name]] = option[:type]
end

$errors = []
def error(file, line, message)
  $errors.push({ file: file, line: (line || -1) + 1, message: message })
end

# Now grep config.cpp for valid JSON read/writes:

json_assignments = []
json_reads = []

config_cpp = File.join(DIR, "src", "config.cpp");
File.foreach(config_cpp).with_index do |line, i|
  # Check Read
  if line =~ /Config\.(\w+)\s*=\s*doc\["(\w+)"\](\s*\|\s*\S+)?/
    json_reads << $2
    type = config_values[$1]
    if $1 != $2
      error config_cpp, i, "JSON read #{$2.inspect} does not match config key #{$1.inspect}"
    end
    if type.nil?
      error config_cpp, i, "JSON read #{$2.inspect} to unknown key"
    end
    if $3.nil? || $3 == ""
      error config_cpp, i, "JSON read #{$2.inspect} missing default value for #{type.inspect}"
    end
    if type =~ /String|char\[\d+\]/
      error config_cpp, i, "JSON assigns #{$2.inspect} to string type! Use strlcpy!"
    end
  end

  # Check String Reads
  if line =~ /strlcpy\s*\(\s*Config\.(\w+)\s*,\s*doc\["(\w+)"\]\s*(|.*?)?,\s*sizeof\s*\(\s*Config\.(\w+)/
    type = config_values[$1]
    json_reads << $2
    if $1 != $2
      error config_cpp, i, "JSON read #{$2.inspect} does not match config key #{$1.inspect}"
    end
    if $1 != $4
      error config_cpp, i, "JSON read #{$2.inspect} has mismatched config key #{$4.inspect} in sizeof() call"
    end
    if type.nil?
      error config_cpp, i, "JSON read #{$2.inspect} to unknown key"
    end
    if $3.nil? || $3 == ""
      error config_cpp, i, "JSON read #{$2.inspect} missing default value for #{type.inspect}"
    end
    if type !~ /String|char\[\d+\]/
      error config_cpp, i, "JSON assigns #{$2.inspect} as string, but the type should be #{type.inspect}"
    end
  end

  # Check Assignment
  if line =~ /doc\s*\[\"(\w+)\"\]\s*=\s*Config\.(\w+)/
    type = config_values[$2]
    json_assignments << $1
    if $1 != $2
      error config_cpp, i, "JSON assignment #{$1.inspect} does not match config key #{$2.inspect}"
    end
    if type.nil?
      error config_cpp, i, "JSON assignment #{$1.inspect} from unknown key"
    end
  end
end

# Check Missing Assignments
(config_values.keys - json_assignments).each do |value|
  error config_cpp, nil, "Missing JSON assignment for #{value.inspect}"
end

# Check Missing Reads
(config_values.keys - json_reads).each do |value|
  error config_cpp, nil, "Missing JSON read for #{value.inspect}"
end

console_checks = []
console_gets = []
console_sets = []
last_check = ""

# Check Console.cpp, which will soon move to config.cpp
console_cpp = File.join(DIR, "src", "Console.cpp")
File.foreach(console_cpp).with_index do |line, i|
  if line =~ /strcmp\s*\(\s*option\s*,\s*"(\w+)"/
    console_checks << $1
    last_check = $1
  end

  if line =~ /String\s*\(\s*Config\.(\w+)/
    console_gets << $1
    type = config_values[$1]
    if ($1 != last_check)
      error console_cpp, i, "Config value #{$1.inspect} does not match last checked key #{last_check.inspect}"
    end
    if type.nil?
      error console_cpp, i, "Read unknown key #{$1.inspect} from Config"
    end
  end

  if line =~ /Config\.(\w+)\s*=\s*(.*?);/
    console_sets << $1
    type = config_values[$1]
    if ($1 != last_check)
      error console_cpp, i, "Config value #{$1.inspect} does not match last checked key #{last_check.inspect}"
    end
    if type.nil?
      error console_cpp, i, "Setting unknown key #{$1.inspect} to Config"
    end
    option = $1
    setter = $2
    case type
    when "bool"
      if $2 !~ /atob\s*\(/
        error console_cpp, i, "Config option #{option.inspect} is #{type}, use atob()"
      end
    when /int|byte|char/
      if setter !~ /atoi\s*\(/
        error console_cpp, i, "Config option #{option.inspect} is #{type}, use atoi()"
      end
    else
      error console_cpp, i, "Config option #{option.inspect} using invalid setter"
    end
  end

  if line =~ /strlcpy\s*\(\s*Config\.(\w+)\s*,\s*args\[1\]\s*,\s*sizeof\s*\(\s*Config\.(\w+)/
    type = config_values[$1]
    console_sets << $1
    lval = $1
    if $1 != $2
      error console_cpp, i, "Console read #{$1.inspect} has mismatched config key #{$2.inspect} in sizeof() call"
    end
    if ($1 != last_check)
      error console_cpp, i, "Config value #{$1.inspect} does not match last checked key #{last_check.inspect}"
    end
    if type.nil?
      error console_cpp, i, "Console read #{$1.inspect} to unknown key"
    end
    if type !~ /String|char\[\d+\]/
      error console_cpp, i, "Console assigns #{lval.inspect} as string, but the type should be #{type.inspect}"
    end
  end
end

# Check Missing Checks
(config_values.keys - console_checks).each do |value|
  error console_cpp, nil, "Missing Console check for #{value.inspect}"
end

# Check Missing Gets
(config_values.keys - console_gets).each do |value|
  error console_cpp, nil, "Missing Console get for #{value.inspect}"
end

# Check Missing Sets
(config_values.keys - console_sets).each do |value|
  error console_cpp, nil, "Missing Console set for #{value.inspect}"
end

puts
# error count
if $errors.length > 0
  $errors.each do |err|
    puts "IN #{err[:file]}:#{err[:line] || 0}\n  > #{err[:message]}"
  end
  puts
  puts "#{$errors.length} errors!"
else
  puts "No errors!"
end