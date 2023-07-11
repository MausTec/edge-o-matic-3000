#!/usr/bin/env ruby

require 'open3'
require 'fileutils'
require 'rubyserial'
require 'tty-prompt'
require 'timeout'

require_relative './screenshot.rb'

class ESPTool
  ROOT_PATH = File.absolute_path(File.join(File.dirname(__FILE__), "..", "..")).freeze
  ESPTOOL_PATH = 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\3.0.0'.freeze
  HARDWARE_PATH = 'C:\Users\eiser.000\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6'.freeze
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

    code_image = File.join(ROOT_PATH, "build", "arduino", 'nogasm-wifi.ino.bin')
    partitions_image = File.join(ROOT_PATH, "build", "arduino", 'nogasm-wifi.ino.partitions.bin')

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

    Open3::popen3(ESPTOOL_BIN, *args) do |sin, sout, serr, wait_thr|
      sout.each_line { |l| puts l }
      serr.each_line { |l| puts l }

      unless wait_thr.value.success?
        throw "EXIT #{wait_thr.value}"
      end
    end
  end

  def set_serial(serial_no)
    serial = Serial.new(@port, @baud)
    read_serial(serial, /^READY/)
    serial.write(".setser #{serial_no}\n")
    read_serial(serial, /^OK|^ERR_/)
    serial.write(".getser\n")
    validated = read_serial(serial, /Serial:\s*(\S+)/)
    serial.close

    if validated == serial_no
      puts "Serial set: %s" % [validated]
      return true
    else
      puts "Serial Mismatch! Read: %s, Wrote: %s" % [validated.inspect, serial_no.inspect]
      return false
    end
  end

  def console!
    serial = Serial.new(@port, @baud)
    read_thr = Thread.new do
      in_stack_trace = false
      stack_trace = []
      loop do
        begin
          line = serial.gets

          if line =~ /[A-Za-z0-9]{120,}/
            save_screenshot(line.chomp)
          else
            puts line
          end

          if line =~ /abort\(\) was called|Guru Meditation Error/
            stack_trace = []
            in_stack_trace = true
          end

          if in_stack_trace
            stack_trace.push line
          end

          if line =~ /Backtrace:/
            in_stack_trace = false
            puts decode_exception(stack_trace.join("\n"))
          end
        rescue RubySerial::Error => e
          if Thread.current.report_on_exception
            read_thr.report_on_exception = false
            Thread.current.abort_on_exception = true
            raise e
          end
          break
        end
      end
    end
    loop do
      line = gets
      serial.write line
    end
  rescue RubySerial::Error => e
    puts "Disconnected: " + e.message
  rescue Interrupt
    read_thr.report_on_exception = false
    puts "Disconnecting..."
    serial.close
    read_thr.join
  ensure
    serial&.close rescue nil
  end

  def put_file(local_path, remote_path)
    serial = Serial.new(@port, @baud)
    serial.write("fput \"#{remote_path}\"\n")
    read_serial(serial, /^>>>SENDHEX:\s+/)
    puts "Sending to #{$1}"
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
    1.upto 255 do |index|
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
      ignored_ports = ['COM1', 'COM5']

      # Reject COM1, maybe you don't want this
      if found_ports.length > 1
        found_ports.reject! {|k,v| ignored_ports.include? k}
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

  def decode_exception(dump=nil)
    cd = File.join(File.dirname(__FILE__), '..', '..')
    jarfile = File.absolute_path(File.join(cd, 'tmp', 'esp-exception-decoder.jar'))
    addr2line = "C:\\MinGW\\bin\\addr2line.exe"
    exceptiontxt = File.absolute_path(File.join(cd, 'tmp', 'exception.txt'))
    elffile = File.absolute_path(File.join(cd, 'build', 'arduino', 'nogasm-wifi.ino.elf'))

    if dump
      File.unlink(exceptiontxt)
      File.write(exceptiontxt, dump)
    end

    args = [
      '-jar',
      jarfile,
      addr2line,
      elffile,
      exceptiontxt
    ]

    out = []
    err = []

    Open3::popen3("java", *args) do |sin, sout, serr, wait_thr|
      sout.each_line { |l| out << l }
      serr.each_line { |l| err << l }
    end

    [ out, err ]
  end

  def read_serial(serial, match = /^\r\n$/, timeout = 3)
    line = nil
    Timeout::timeout(5) do
      begin
        line = serial.gets
        puts "DEVICE: " + line
      end while line !~ match
      return $1
    end
  rescue Timeout::Error => e
    # nop
    return nil
  end
end