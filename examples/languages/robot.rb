#!/usr/bin/env ruby

require 'json'

file = File.read(ARGV[0])
json = JSON.parse(file)
translated = {}
lang = File.basename(ARGV[0], '.json')

json.each do |key, _|
    puts "Translating \"#{key}\"..."
    translated[key] = `trans -b en:#{lang} \"#{key}\"`.strip
end

final = JSON.pretty_generate(translated)
puts final