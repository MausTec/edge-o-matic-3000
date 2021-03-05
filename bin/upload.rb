#!/usr/bin/env ruby

ROOT_PATH = File.absolute_path(File.join(File.dirname(__FILE__), "..")).freeze
Dir.chdir(ROOT_PATH)

build_root = File.join(ROOT_PATH, "build", "arduino")
release_root = File.join(ROOT_PATH, "tmp", "release")

require_relative './lib/esptool.rb'
require_relative './lib/arduino.rb'
require 'optimist'
require 'semantic'
require 'yaml'

opts = Optimist::options do
  banner <<-TEXT
Compile and upload this code to the ESP, using the Arduino environment for now.
TEXT

  opt :port, "Serial port to upload to", type: :string, default: 'auto'
  opt :ino_file, "Path to main .ino", type: :string, default: File.join(ROOT_PATH, "nogasm-wifi.ino")
  opt :compile, "Recompile the software before upload", type: :bool, default: false
  opt :upload, "Upload to the device", type: :bool, default: false
  opt :get_version, "Show software version and exit", type: :bool, default: false
  opt :set_version, "Manually set the current version", type: :string, default: nil
  opt :inc_version, "Increase major|minor|patch", type: :string, default: nil
  opt :tag, "Tag this as a release and push", type: :bool, default: false
  opt :serial, "Set a serial number for this device", type: :string, default: nil
  opt :serial_prefix, "Autogen a serial from serials.yml", type: :string, default: nil
  opt :file_path, "Copy binary to a file path for update", type: :string, default: nil
  opt :console, "Open a console to the device", type: :bool, default: false
end

def get_version
  $version ||= begin
      (tag, n, sha) = `git describe --tags`.split('-')
      v = Semantic::Version.new(tag.gsub(/^v/, ''))
      if n.to_i > 0
        v.build = sha
      end
      v
  end
end

def set_version(v)
  data = "#define VERSION \"#{v.to_s}\"\n"
  File.write(File.join(ROOT_PATH, "VERSION.h"), data)
  $version = v
  puts "Set version to: #{v.to_s}"
end

def sh(cmd)
  puts "$ #{cmd}"
  puts `#{cmd}`
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
  sh "git tag v#{v.to_s}"
  sh "git push"
  sh "git push --tags"

  file_friendly_version = v.to_s.gsub(/\+/, '-')

  FileUtils.rm_rf(release_root)
  FileUtils.mkdir_p(release_root)
  FileUtils.cp(File.join(build_root, "nogasm-wifi.ino.bin"), File.join(release_root, "eom3k-#{file_friendly_version}.bin"))
  FileUtils.cp(File.join(build_root, "nogasm-wifi.ino.partitions.bin"), File.join(release_root, "eom3k-#{file_friendly_version}.partitions.bin"))

  sh "gh release create v#{file_friendly_version} #{File.join(release_root, "eom3k-#{file_friendly_version}.bin")} #{File.join(release_root, "eom3k-#{file_friendly_version}.partitions.bin")} --notes-file #{File.join(ROOT_PATH, "doc", "ReleaseTemplate.md")}"
end

esptool = nil

if opts[:port] && (opts[:upload] || opts[:serial] || opts[:console] || opts[:serial_prefix])
  esptool = ESPTool.new(opts[:port])
end

if opts[:upload]
  esptool&.write_flash
end

if opts[:serial]
  esptool&.set_serial(opts[:serial])
end

if (prefix = opts[:serial_prefix])
  serials = YAML.load(File.read("serials.yml"));
  last_ser = serials[prefix]
  last_ser += 1
  if esptool&.set_serial("%s-%02d%03d" % [prefix, Date.today.year % 100, last_ser])
    serials[prefix] = last_ser
    File.write("serials.yml", YAML.dump(serials))
  end
end

if opts[:file_path]
  update_bin_path = File.join(build_root, "nogasm-wifi.ino.bin")
  target_update_path = File.join(opts[:file_path], "update.bin")
  puts "Copying #{update_bin_path} to #{target_update_path}"
  FileUtils.cp(update_bin_path, target_update_path)
end

if opts[:console]
  esptool&.console!
end