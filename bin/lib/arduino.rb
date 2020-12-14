#!/usr/bin/env ruby

require 'open3'
require 'fileutils'

#C:\Program Files (x86)\Arduino\arduino-builder -dump-prefs -logger=machine
#-hardware C:\Program Files (x86)\Arduino\hardware
#-hardware C:\Users\eiser.000\AppData\Local\Arduino15\packages
#-tools C:\Program Files (x86)\Arduino\tools-builder
#-tools C:\Program Files (x86)\Arduino\hardware\tools\avr
#-tools C:\Users\eiser.000\AppData\Local\Arduino15\packages
  #-built-in-libraries C:\Program Files (x86)\Arduino\libraries
  #-libraries E:\Documents\Arduino\libraries
  #-fqbn=esp32:esp32:esp32:PSRAM=disabled,PartitionScheme=min_spiffs,CPUFreq=240,FlashMode=qio,FlashFreq=80,FlashSize=4M,UploadSpeed=921600,DebugLevel=none
  #-vid-pid=0403_6015
  #-ide-version=10813
  #-build-path C:\Users\eiser.000\AppData\Local\Temp\arduino_build_233518
  #-warnings=none
#-prefs=build.warn_data_percentage=75
#-prefs=runtime.tools.esptool_py.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\2.6.1
#-prefs=runtime.tools.esptool_py-2.6.1.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\2.6.1
#-prefs=runtime.tools.mkspiffs.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\mkspiffs\0.2.3
#-prefs=runtime.tools.mkspiffs-0.2.3.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\mkspiffs\0.2.3
#-prefs=runtime.tools.xtensa-esp32-elf-gcc.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\xtensa-esp32-elf-gcc\1.22.0-80-g6c4433a-5.2.0
#-prefs=runtime.tools.xtensa-esp32-elf-gcc-1.22.0-80-g6c4433a-5.2.0.path=C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\xtensa-esp32-elf-gcc\1.22.0-80-g6c4433a-5.2.0
  #-verbose
#E:\Documents\Arduino\ESP32_WiFi\ESP32_WiFi.ino

class Arduino
  class Builder
    ROOT_PATH = File.absolute_path(File.join(File.dirname(__FILE__), "..", "..")).freeze
    BUILDER_PATH = 'C:\Program Files (x86)\Arduino\arduino-builder'

    def initialize(ino_file, builder_opts={}, esp_opts={})
      system_lib_path = 'C:\Program Files (x86)\Arduino\libraries'
      local_lib_path = 'E:\Documents\Arduino\libraries'
      build_path = File.join(ROOT_PATH, 'build/arduino')

      @esp_opts = {
        PSRAM: 'disabled',
        PartitionScheme: 'min_spiffs',
        CPUFreq: 240,
        FlashMode: 'qio',
        FlashFreq: 80,
        FlashSize: '4M',
        UploadSpeed: 921600,
        DebugLevel: 'none'
      }.merge(esp_opts)

      @prefs = {
        'HTTPS_DISABLE_SELFSIGNING' => 'true',
        'build.warn_data_percentage' => '75',
        'runtime.tools.esptool_py.path' => 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\2.6.1',
        'runtime.tools.esptool_py-2.6.1.path' => 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\2.6.1',
        'runtime.tools.mkspiffs.path' => 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\mkspiffs\0.2.3',
        'runtime.tools.mkspiffs-0.2.3.path' => 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\mkspiffs\0.2.3',
        'runtime.tools.xtensa-esp32-elf-gcc.path' => 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\xtensa-esp32-elf-gcc\1.22.0-80-g6c4433a-5.2.0',
        'runtime.tools.xtensa-esp32-elf-gcc-1.22.0-80-g6c4433a-5.2.0.path' => 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\xtensa-esp32-elf-gcc\1.22.0-80-g6c4433a-5.2.0'
      }

      @tools = [
        'C:\Program Files (x86)\Arduino\tools-builder',
        'C:\Program Files (x86)\Arduino\hardware\tools\avr',
        'C:\Users\eiser.000\AppData\Local\Arduino15\packages'
      ]

      @hardware = [
        'C:\Program Files (x86)\Arduino\hardware',
        'C:\Users\eiser.000\AppData\Local\Arduino15\packages'
      ]

      @ino_file = ino_file

      @builder_opts = {
        #logger: 'machine',
        verbose: true,
        built_in_libraries: system_lib_path,
        libraries: local_lib_path,
        build_path: build_path,
        warnings: 'none',
        ide_version: '10813',
        vid_pid: '0403_6015'
      }.merge(builder_opts)
    end

    def dump_prefs
      command = 'dump-prefs'
      args = generate_args(command)
      FileUtils.mkdir_p(@builder_opts[:build_path])
      Open3::popen3(BUILDER_PATH, *args) do |sin, sout, serr|
        sout.each_line { |l| puts l }
        serr.each_line { |l| puts l }
      end
    end

    def compile
      command = 'compile'
      args = generate_args(command)
      Open3::popen3(BUILDER_PATH, *args) do |sin, sout, serr, wait_thr|
        sout.each_line { |l| puts l }
        serr.each_line { |l| puts l }
        exit_status = wait_thr.value
        if !exit_status.success?
          raise "EXIT #{exit_status}"
        end
      end
    end

  private

    def generate_fqbn
      'esp32:esp32:esp32:' + @esp_opts.collect { |k,v| "#{k}=#{v}" }.join(',')
    end

    def generate_args(command)
      args = ['-' + command]

      @hardware.each do |hardware|
        args << '-hardware'
        args << hardware
      end

      @tools.each do |tool|
        args << '-tools'
        args << tool
      end

      args << "-fqbn=#{generate_fqbn}"

      @builder_opts.each do |k, v|
        args << '-' + k.to_s.gsub(/_/, '-')
        args << v.to_s unless v === true
      end

      @prefs.each do |k, v|
        args << "-prefs=#{k}=#{v}"
      end

      args << @ino_file

      puts args.join(' ')

      return args
    end
  end
end