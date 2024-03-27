#!/usr/bin/env ruby

require 'json'

file = File.read(ARGV[0])
json = JSON.parse(file)
translated = {}

json['keys'].each do |key, _|
    puts "Translating \"#{key}\"..."
    translated[key] = `trans -b en:#{json['code'].downcase} \"#{key}\"`.strip
end

final = JSON.pretty_generate({
    language: json['language'],
    code: json['code'],
    keys: translated
})

puts final