#!/usr/bin/env ruby

ROOT_PATH = File.absolute_path(File.join(File.dirname(__FILE__), "..")).freeze
Dir.chdir(ROOT_PATH)

require_relative './lib/esptool.rb'
require_relative './lib/arduino.rb'
require 'optimist'
require 'semantic'

opts = Optimist::options do
  banner <<-TEXT
Compile and upload this code to the ESP, using the Arduino environment for now.
TEXT

  opt :port, "Serial port to upload to", type: :string, default: 'auto'
  opt :ino_file, "Path to main .ino", type: :string, default: File.join(ROOT_PATH, "ESP32_WiFi.ino")
  opt :compile, "Recompile the software before upload", type: :bool, default: false
  opt :upload, "Upload to the device", type: :bool, default: true
  opt :get_version, "Show software version and exit", type: :bool, default: false
  opt :set_version, "Manually set the current version", type: :string, default: nil
  opt :inc_version, "Increase major|minor|patch", type: :string, default: nil
  opt :tag, "Tag this as a release and push", type: :bool, default: false
  opt :serial, "Set a serial number for this device", type: :string, default: nil
end

def get_version
  version_h = File.read(File.join(ROOT_PATH, "VERSION.h"))
  if version_h =~ /#define\s*VERSION\s*"(.*?)"/
    v = Semantic::Version.new($1)
    return v
  else
    raise "Unable to parse VERSION.h"
  end
end

def set_version(v)
  data = "#define VERSION \"#{v.to_s}\"\n"
  File.write(File.join(ROOT_PATH, "VERSION.h"), data)
  puts "Set version to: #{v.to_s}"
end

if opts[:get_version]
  puts get_version.to_s
  exit
end

if opts[:inc_version]
  v = get_version
  new_v = v.increment!(opts[:inc_version])
  set_version(new_v)
elsif opts[:set_version]
  v = Semantic::Version.new(opts[:set_version])
  set_version(v)
end

if opts[:compile]
  ino_file = opts[:ino_file]
  builder = Arduino::Builder.new(ino_file)
  builder.compile
end

if opts[:tag]
  v = get_version
  puts `git commit VERSION.h -m "Set version to #{v.to_s}"`
  puts `git tag v#{v.to_s}`
  puts `git push`
  puts `git push --tags`

  build_root = File.join(ROOT_PATH, "build", "arduino")
  release_root = File.join(ROOT_PATH, "tmp", "release")

  FileUtils.rm_rf(release_root)
  FileUtils.mkdir_p(release_root)
  FileUtils.cp(File.join(build_root, "ESP32_WiFi.ino.bin"), File.join(release_root, "eom3k-#{v.to_s}.bin"))
  FileUtils.cp(File.join(build_root, "ESP32_WiFi.ino.partitions.bin"), File.join(release_root, "eom3k-#{v.to_s}.partitions.bin"))

  `gh release create v#{v.to_s} #{File.join(release_root, "eom3k-#{v.to_s}.bin")} #{File.join(release_root, "eom3k-#{v.to_s}.partitions.bin")} --notes-file #{File.join(ROOT_PATH, "doc", "ReleaseTemplate.md")}`
end

esptool = nil

if opts[:port]
  esptool = ESPTool.new(opts[:port])
end

if opts[:upload]
  esptool&.write_flash
end

if opts[:serial]
  esptool&.set_serial(opts[:serial])
end