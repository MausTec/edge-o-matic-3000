#!/usr/bin/env ruby

require 'open3'
require 'fileutils'
require 'rubyserial'
require 'tty-prompt'
require 'timeout'

class ESPTool
  ROOT_PATH = File.absolute_path(File.join(File.dirname(__FILE__), "..", "..")).freeze
  ESPTOOL_PATH = 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\2.6.1'.freeze
  HARDWARE_PATH = 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4'.freeze
  ESPTOOL_BIN  = File.join(ESPTOOL_PATH, "esptool.exe").freeze

  def initialize(port, bin_dir=ESPTOOL_PATH, bin_options={})
    @bin_dir = bin_dir

    @port = select_port(port)
    @baud = 115200

    @bin_options = {
        chip: 'esp32',
        port: @port,
        baud: '921600',
        before: 'default_reset',
        after: 'hard_reset'
    }.merge(bin_options)
  end

  def write_flash(opts = {})
    options = {
        flash_mode: 'dio',
        flash_freq: '80m',
        flash_size: 'detect',
        z: true
    }.merge(opts)

    args  = build_opts_array(@bin_options)
    args << "write_flash"
    args += build_opts_array(options)

    code_image = File.join(ROOT_PATH, "build", "arduino", 'ESP32_WiFi.ino.bin')
    partitions_image = File.join(ROOT_PATH, "build", "arduino", 'ESP32_WiFi.ino.partitions.bin')

    images = {
        '0xe000': File.join(HARDWARE_PATH, "tools", "partitions", "boot_app0.bin"),
        '0x1000': File.join(HARDWARE_PATH, "tools", "sdk", "bin", "bootloader_qio_80m.bin"),
        '0x8000': partitions_image,
        '0x10000': code_image
    }

    images.each do |addr, image|
      args.push(addr.to_s)
      args.push(image)
    end

    Open3::popen3(ESPTOOL_BIN, *args) do |sin, sout, serr|
      sout.each_line { |l| puts l }
      serr.each_line { |l| puts l }
    end
  end

  def set_serial(serial_no)
    serial = Serial.new(@port, @baud)
    read_serial(serial, /^READY/)
    serial.write(".setser #{serial_no}\n")
    read_serial(serial, /^OK|^ERR_/)
    serial.write(".getser\n")
    read_serial(serial, /Serial:/)
    serial.close
  end

private
  def build_opts_array(options={})
    opts_array = []
    options.each do |key, value|
      key_s = key.to_s
      prefix = "--"

      if key_s.length == 1
        prefix = "-"
      end

      opts_array.push(prefix + key_s)
      opts_array.push(value.to_s) unless value === true
    end
    return opts_array
  end

  def search_ports_win
    ports = {}
    1.upto 64 do |index|
      begin
       serial = Serial.new  portname = 'COM' + index.to_s
        ports[portname] = 'is available' if serial
        serial.close
      rescue  Exception => e
        ports[portname] = 'access denied' if e.to_s.include? "ACCESS_DENIED"
      end
    end
    return ports
  end

  def select_port(default_port)
    prompt = TTY::Prompt.new
    port = default_port

    if port == 'auto'
      found_ports = search_ports_win

      # Reject COM1, maybe you don't want this
      if found_ports.length > 1
        found_ports.reject! {|k,v| k == 'COM1'}
      end

      if found_ports.length > 1
        port = prompt.select("What Port:") do |menu|
          search_ports_win.each do |port, status|
            disabled = status !~ /available/ ? status : nil
            menu.choice name: "#{port}", value: port, disabled: disabled
          end
        end
      else
        port = found_ports.keys.first
      end
    end

    port
  end

  def read_serial(serial, match = /^\r\n$/, timeout = 3)
    line = nil
    Timeout::timeout(5) do
      begin
        line = serial.gets
        puts "DEVICE: " + line
      end while line !~ match
    end
  rescue Timeout::Error => e
    # nop
  end
end