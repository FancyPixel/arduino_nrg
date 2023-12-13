#!/usr/bin/env ruby

### This script combines the RF bootloader and the compiled sketch into a single .hex

require 'fileutils'

MSP430_BSL_GEM_VERSION='0.3.0'.freeze

unless system "gem list msp430_bsl -v #{MSP430_BSL_GEM_VERSION} -i --silent"
  puts "Installing msp430_bsl-#{MSP430_BSL_GEM_VERSION}"
  system "gem install msp430_bsl -v #{MSP430_BSL_GEM_VERSION}"
end

must_execute = ARGV.shift == 'true'

# Break if we've not enabled "Wireless bootloader" ARDUINO IDE's option
unless must_execute
  exit 0
end

puts "\n\nCombining RF bootloader and sketch in a single file\n\n"

SKETCH_HEX_FILE_PATH = ARGV.shift

HEX_DATA_LINE_LEN = 16
VECTOR_TABLE_ADDR = 0xFF80
BOOTLOADER_STARTING_ADDR = 0x8000
BOOT_HEX_FILE_PATH = File.expand_path File.join(__dir__, '../', 'rfloader.hex')

class Numeric
  def to_hex_str
    n = to_s(16).upcase
    if n.length.odd?
      n = "0#{n}"
    end
    n
  end
end

def calculate_crc(line, has_crc = true)
  sum = 0
  last_char = has_crc ? -3 : -1
  line[1..last_char].chars.each_slice(2) { |byte| sum += byte.join().to_i(16) }
  ((~sum + 1) & 0xFF)
end

def addr_of_line(line)
  line.strip[3..6].to_i(16)
end

def data_length_of_line(line)
  line.strip[1..2].to_i(16)
end

# Read SKETCH_HEX - We need to know the starting address
sketch_lines = File.read(SKETCH_HEX_FILE_PATH).lines
user_code_starting_addr = addr_of_line(sketch_lines.first)

# Combine bootloader and sketch hexes
combined_file_content = []
File.read(BOOT_HEX_FILE_PATH).each_line do |line|
  line.strip!
  if !line.start_with?(':00') && !line.start_with?(':10FF') && !line.start_with?(':04000003')
    combined_file_content << line
  end
end

File.read(SKETCH_HEX_FILE_PATH).each_line do |line|
  line.strip!
  addr_str = line[3..6]
  addr = addr_str.to_i(16)
  if addr >= VECTOR_TABLE_ADDR
    if addr == 0xFFB0
      line[-10..-7] = "#{(user_code_starting_addr & 0xFF).to_hex_str}#{((user_code_starting_addr >> 8) & 0xFF).to_hex_str}"
      line[-2..-1] = calculate_crc(line).to_hex_str
    end

    if addr == 0xFFF0
      line[-6..-3] = "#{(BOOTLOADER_STARTING_ADDR & 0xFF).to_hex_str}#{((BOOTLOADER_STARTING_ADDR >> 8) & 0xFF).to_hex_str}"
      line[-2..-1] = calculate_crc(line).to_hex_str
    end

    combined_file_content << line
  elsif !line.start_with?(':04000003')
    combined_file_content << line
  end
end


# Write new file content
f = File.open(SKETCH_HEX_FILE_PATH, 'w+')
f.write combined_file_content.join "\n"
f.close
