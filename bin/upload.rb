#!/usr/bin/env ruby

ROOT_PATH = File.absolute_path(File.join(File.dirname(__FILE__), "..")).freeze
Dir.chdir(ROOT_PATH)

build_root = File.join(ROOT_PATH, ".pio", "build", "esp32dev")
release_root = File.join(ROOT_PATH, "tmp", "release")

require_relative './lib/esptool.rb'
require_relative './lib/arduino.rb'
require 'optimist'
require 'semantic'
require 'yaml'

opts = Optimist::options do
  banner <<-TEXT
Compile and upload this code to the ESP.
TEXT

  opt :port, "Serial port to upload to", type: :string, default: 'auto'
  opt :pio, "Use the PlatformIO build chain", type: :bool, default: true
  opt :ino_file, "Path to main .ino", type: :string, default: File.join(ROOT_PATH, "src/main.cpp")
  opt :compile, "Recompile the software before upload", type: :bool, default: false
  opt :upload, "Upload to the device", type: :bool, default: false
  opt :get_version, "Show software version and exit", type: :bool, default: false
  opt :set_version, "Manually set the current version", type: :string, default: nil
  opt :inc_version, "Increase major|minor|patch", type: :string, default: nil
  opt :tag, "Tag this as a release and push", type: :bool, default: false
  opt :serial, "Set a serial number for this device", type: :string, default: nil
  opt :serial_prefix, "Autogen a serial from serials.yml", type: :string, default: nil
  opt :serial_variant, "Append a veriant to the serial number.", type: :string, default: nil
  opt :file_path, "Copy binary to a file path for update", type: :string, default: nil
  opt :console, "Open a console to the device", type: :bool, default: false
  opt :exception, "Decode the last exception dump", type: :bool, default: false
end

def get_version
  $version ||= begin
      (tag, n, sha) = `git describe --tags`.split('-')
      v = Semantic::Version.new(tag.gsub(/^v/, ''))
      if n.to_i > 0
        v.build = sha.chop
      end
      v
  end
end

def set_version(v)
  data = "#define VERSION \"#{v.to_s}\"\n"
  File.write(File.join(ROOT_PATH, "include", "VERSION.h"), data)
  $version = v
  puts "Set version to: #{v.to_s}"
end

def sh(cmd)
  puts "$ #{cmd}"
  puts `#{cmd}`
end

if opts[:exception]
  tool = ESPTool.new nil
  puts tool.send(:decode_exception, <<-DUMP)
/home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/queue.c:1442 (xQueueGenericReceive)- assert failed!
abort() was called at PC 0x4009002d on core 1

Backtrace: 0x400941ac:0x3ffd1700 0x400943dd:0x3ffd1720 0x4009002d:0x3ffd1740 0x400eb08e:0x3ffd1780 0x400f3bb2:0x3ffd17a0 0x400f3c31:0x3ffd17c0 0x400f3824:0x3ffd17e0 0x400f3961:0x3ffd1820 0x400ee8d2:0x3ffd1840 0x400eea64:0x3ffd1870 0x400e
ef16:0x3ffd1890 0x400e69db:0x3ffd18b0 0x400d38cd:0x3ffd18d0 0x400d38ea:0x3ffd18f0 0x400dfc82:0x3ffd1910 0x401d1c97:0x3ffd1930 0x400d7269:0x3ffd1950 0x400d737b:0x3ffd1980 0x400d9006:0x3ffd19b0 0x400d4f8a:0x3ffd19d0 0x400e4ddd:0x3ffd19f0 0
x400e4e8a:0x3ffd1a10 0x400d4fdb:0x3ffd1a30 0x400d3656:0x3ffd1a50 0x401070ed:0x3ffd1a70 0x40090341:0x3ffd1a90
DUMP
  exit 0
end

if opts[:get_version]
  v = get_version
  puts v.to_s
  set_version(v)
  exit
end

if opts[:inc_version]
  v = get_version
  new_v = v.increment!(opts[:inc_version])
  set_version(new_v)
elsif opts[:set_version]
  v = Semantic::Version.new(opts[:set_version])
  set_version(v)
else
  v = get_version
  set_version(v)
end

if opts[:compile]
  if opts[:pio]
    puts `pio run`
  else
    ino_file = opts[:ino_file]
    builder = Arduino::Builder.new(ino_file)
    builder.compile
  end
end

if opts[:tag]
  v = get_version
  sh "git tag v#{v.to_s}"
  sh "git push"
  sh "git push --tags"

  file_friendly_version = v.to_s.gsub(/\+/, '-')

  FileUtils.rm_rf(release_root)
  FileUtils.mkdir_p(release_root)
  FileUtils.cp(File.join(build_root, "firmware.bin"), File.join(release_root, "eom3k-#{file_friendly_version}.bin"))
  FileUtils.cp(File.join(build_root, "partitions.bin"), File.join(release_root, "eom3k-#{file_friendly_version}.partitions.bin"))

  sh "gh release create v#{file_friendly_version} #{File.join(release_root, "eom3k-#{file_friendly_version}.bin")} #{File.join(release_root, "eom3k-#{file_friendly_version}.partitions.bin")} --notes-file #{File.join(ROOT_PATH, "doc", "ReleaseTemplate.md")}"
end

esptool = nil

if opts[:port] && (opts[:upload] || opts[:serial] || opts[:console] || opts[:serial_prefix])
  esptool = ESPTool.new(opts[:port])
end

if opts[:upload]
  if opts[:pio]
    puts `pio run -t upload`
  else
    esptool&.write_flash
  end
end

if opts[:serial]
  esptool&.set_serial(opts[:serial])
end

if (prefix = opts[:serial_prefix])
  serials = YAML.load(File.read("serials.yml"));
  last_ser = serials[prefix]
  last_ser += 1

  serial = "%s-%02d%03d" % [prefix, Date.today.year % 100, last_ser]
  if (variant = opts[:serial_variant])
    serial += "-" + variant
  end

  if esptool&.set_serial(serial)
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