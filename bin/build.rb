#!/usr/bin/env ruby

require 'open3'
require 'fileutils'
require 'optimist'

ARDUINO_ROOT = 'C:\Program Files (x86)\Arduino'
AVR_BIN = 'C:\Program Files (x86)\Arduino\hardware\tools\avr\bin'

# C:\Program Files (x86)\Arduino\arduino-builder
# -compile -logger=machine
# -hardware C:\Program Files (x86)\Arduino\hardware
# -hardware C:\Users\eiser.000\AppData\Local\Arduino15\packages
# -tools C:\Program Files (x86)\Arduino\tools-builder
# -tools C:\Program Files (x86)\Arduino\hardware\tools\avr
# -tools C:\Users\eiser.000\AppData\Local\Arduino15\packages
# -built-in-libraries C:\Program Files (x86)\Arduino\libraries
# -libraries E:\Documents\Arduino\libraries
# -fqbn=avr_boot:avr:avr_boot_atmega328:cpu=atmega328p,model=uno,clock=16mhz_low_power,SDCS=4,BOD=2_7
# -vid-pid=0000_0000
# -ide-version=10809
# -build-path C:\Users\eiser.000\AppData\Local\Temp\arduino_build_947831
# -warnings=none
# -prefs=build.warn_data_percentage=75
# -prefs=runtime.tools.avr-gcc.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\avr-gcc\4.9.2-atmel3.5.4-arduino2
# -prefs=runtime.tools.avr-gcc-4.9.2-atmel3.5.4-arduino2.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\avr-gcc\4.9.2-atmel3.5.4-arduino2
# -prefs=runtime.tools.avrdude.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino9
# -prefs=runtime.tools.avrdude-6.3.0-arduino9.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino9
# -prefs=runtime.tools.arduinoOTA.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\arduinoOTA\1.1.1
# -prefs=runtime.tools.arduinoOTA-1.1.1.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\arduinoOTA\1.1.1
# -verbose E:\Documents\Arduino\ThermalDetonator\ThermalDetonator.ino


opts = Optimist::options do
  banner <<-TEXT
Export firmware image and assets to an SD card image.
TEXT

  opt :out, "Target to write files to", type: :string
  opt :assets, "Include assets", default: true
  opt :avr_bin, "Path to avr bin directory", default: AVR_BIN
end

class AVR
  class AVRError < RuntimeError; end

  def initialize(bin_dir = AVR_BIN)
    @bin_dir = bin_dir
  end

  def objcopy(input_file:, output_file:, output_type: "binary", input_type: "ihex")
    args = ["-I", input_type, "-O", output_type, input_file, output_file]

    puts "Generating FIRMWARE.BIN"
    stdout, stderr, status = Open3.capture3(AVR_BIN + "\\avr-objcopy.exe", *args)
    puts stdout
    puts stderr
    puts "EXIT: #{status}"

    if status != 0
      raise AVRError, stderr
    end
  end

  def compile(input_file:, verbose: false)
    options = {
      logger: 'machine',
      "built-in-libraries": 'C:\Program Files (x86)\Arduino\libraries',
      libraries: 'E:\Documents\Arduino\libraries',
      fqbn: 'avr_boot:avr:avr_boot_atmega328:cpu=atmega328p,model=uno,clock=16mhz_low_power,SDCS=4,BOD=2_7',
      "build-path": File.expand_path("tmp"),
    }

    hardware = [
        'C:\Program Files (x86)\Arduino\hardware',
        'C:\Users\eiser.000\AppData\Local\Arduino15\packages',
    ]

    tools = [
        'C:\Program Files (x86)\Arduino\tools-builder',
        'C:\Program Files (x86)\Arduino\hardware\tools\avr',
        'C:\Users\eiser.000\AppData\Local\Arduino15\packages',
    ]

    prefs = {
        "build.warn_data_percentage": 75,
        "runtime.tools.avr-gcc.path": 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\avr-gcc\4.9.2-atmel3.5.4-arduino2',
        "runtime.tools.avr-gcc-4.9.2-atmel3.5.4-arduino2.path": 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\avr-gcc\4.9.2-atmel3.5.4-arduino2',
        "runtime.tools.avrdude.path": 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino9',
        "runtime.tools.avrdude-6.3.0-arduino9.path": 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino9',
        "runtime.tools.arduinoOTA.path": 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\arduinoOTA\1.1.1',
        "runtime.tools.arduinoOTA-1.1.1.path": 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\arduino\tools\arduinoOTA\1.1.1'
    }

    args = ["-compile", *(options.collect { |k, v| ["-" + k.to_s, v.to_s] }.flatten)]

    hardware.each do |hw|
        args << '-hardware'
        args << hw
    end

    tools.each do |tool|
        args << '-tools'
        args << tool
    end

    prefs.each do |key, val|
        args << '-prefs=' + key.to_s + "=" + val.to_s
    end

    args << File.expand_path(input_file)

    if verbose
        args << '-verbose'
    end

    stdout, stderr, status = Open3.capture3(ARDUINO_ROOT + "\\arduino-builder", *args)

    stdout.split("\n").each do |line|
        if line =~ /===(\w+)\s\|\|\|\s+(.*?)\s\|\|\|\s\[(.*?)\]/
            level = $1
            template = $2
            subs = $3.split(/\s+/)

            template.gsub! /\{(\d+)\}/ do |m|
                subs[$1.to_i] || "-"
            end

            template.gsub! /%[%A-F0-9]+/i do |m|
                m == "%%" ? "%" : URI.decode(m)
            end

            print "%-5s - %s" % [level.upcase, template]

            if template =~ /Progress/
                print "\r"
            else
                print "\n"
            end
        else
            puts line
        end
    end

    puts "EXIT: #{status}"

    if status != 0
      raise AVRError, stderr
    end
  end
end

avr = AVR.new(opts[:avr_bin])
avr.compile input_file: "ESP32_WiFi.ino"
avr.objcopy input_file: "tmp/ESP32_WiFi.ino.hex",
            output_file: "build/FIRMWARE.BIN"

if opts[:out]
  target = opts[:out] + "\\FIRMWARE.BIN"
  puts "Copying to #{target}"
  FileUtils.cp("build/FIRMWARE.BIN", target)

  if opts[:assets]
    Dir.glob("assets/**/*").each do |file|
      target = file.gsub("assets", opts[:out])
      puts "CP #{file} -> #{target}"
      if File.directory? file
        FileUtils.mkdir_p(target)
      else
        FileUtils.cp(file, target)
      end
    end
  end
end