#!/usr/bin/env ruby

require 'rmagick'
require 'fileutils'

def save_screenshot(buffer_data)
  puts "Got #{buffer_data.length / 2} bytes."

  buffer = []
  buffer_data.scan(/..?/) do |data|
    buffer.push data
  end

  height = 64
  width = 128

  col_height = ((height + 7) / 8).floor # In Pages

  pages = []

  0.upto(col_height-1) do |page|
    page_start = page * width
    page_data_hex = buffer[page_start...page_start+width]
    page_data = page_data_hex.map { |d| d.to_i(16) }

    columns = []

    0.upto(width-1) do |column|
      rows = []
      byte = page_data[column]

      0.upto(7) do |row|
        rows.push((byte & (1 << (7 - (row % 8)))) != 0)
      end

      columns.unshift rows
    end

    columns.each_with_index do |col, i|
      pages[i] ||= []
      pages[i].unshift *col
    end
  end

  data = pages#.transpose

  scale = 10
  foreground = [0,222,230]
  background = [0,0,0]

  padding = scale * 2
  img = Magick::Image.new(width*scale+padding, height*scale+padding)
  img.background_color = "rgb(#{background.join(',')})"

  if padding > 0
    0.upto(height*scale+padding) do |y|
      0.upto(width*scale+padding) do |x|
        img.pixel_color(x,y, "rgb(#{background.join(',')})")
      end
    end
  end

  data.each_with_index do |row, row_index|
    row.each_with_index do |item, column_index|
      0.upto(scale-1) do |xshift|
        0.upto(scale-1) do |yshift|
          if (scale > 1 && ((xshift == scale - 1) || (yshift == scale - 1)))
            color = background
          else
            color = item ? foreground : background
          end
          img.pixel_color(row_index*scale+xshift+((padding+1)/2).ceil, column_index*scale+yshift+((padding+1)/2).ceil, "rgb(#{color.join(', ')})")
        end
      end
    end
  end

  folder = File.join(File.dirname(__FILE__), "../..", "tmp")
  FileUtils.mkdir_p(folder)
  filename = File.join(folder, "screenshot-#{Time.now.to_i}.bmp")
  img.write(filename)
  puts "Screenshot: #{File.absolute_path(filename)}"
end