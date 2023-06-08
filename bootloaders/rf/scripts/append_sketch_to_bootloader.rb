### This script combines the RF bootloader and the compiled sketch into a single .hex

require 'bundler/setup'
Bundler.require

require 'fileutils'

must_execute = ARGV.shift == 'true'

# Break if we've not enabled "Wireless bootloader" ARDUINO IDE's option
unless must_execute
  exit 0
end

puts "\n\nCombining RF bootloader and sketch in a single file\n\n"

source_files_path = ARGV.shift
build_path = ARGV.shift

HEX_DATA_LINE_LEN = 16
VECTOR_TABLE_ADDR = 0xFF80
BOOTLOADER_STARTING_ADDR = 0x8000
BOOT_HEX_FILE_PATH = File.expand_path File.join(__dir__, '../', 'rfloader.hex')

SKETCH_FILE_NAMES = Dir[File.join build_path, '*.ino.hex']
if SKETCH_FILE_NAMES.size > 1
  puts "ERROR: found more than one sketch: #{SKETCH_FILE_NAMES}"
  exit -1
end

class Numeric
  def to_hex_str
    n = to_s(16).upcase
    if n.length.odd?
      n = "0#{n}"
    end
    n
  end
end

def calculate_crc(line)
  sum = 0
  line[1..-3].chars.each_slice(2) { |byte| sum += byte.join().to_i(16) }
  ((~sum + 1) & 0xFF)
end

def addr_of_line(line)
  line.strip[3..6].to_i(16)
end

def data_length_of_line(line)
  line.strip[1..2].to_i(16)
end

SKETCH_HEX_FILE_PATH = File.expand_path SKETCH_FILE_NAMES.first

# Read SKETCH_HEX - We need to know the starting address
sketch_lines = File.read(SKETCH_HEX_FILE_PATH).lines
user_code_starting_addr = addr_of_line(sketch_lines.first)

# Combine bootloader and sketch hexes
combined_file_content = []
File.read(BOOT_HEX_FILE_PATH).each_line do |line|
  line.strip!
  if !line.start_with?(':00') && !line.start_with?(':10FF') && !line.start_with?(':04000003')
    combined_file_content << line
  elsif line.start_with?(':04000003')
    # Time to pad bootloader!
    # Calculate bootloader's end memory address
    boot_end_memory_addr = addr_of_line(combined_file_content.last) + data_length_of_line(combined_file_content.last)
    total_pad_length = user_code_starting_addr - boot_end_memory_addr

    curr_memory_addr = boot_end_memory_addr
	(total_pad_length / HEX_DATA_LINE_LEN.to_f).ceil.times do |n|
	  remaining_pad = total_pad_length - (n * HEX_DATA_LINE_LEN)
	  curr_pad_length = remaining_pad < HEX_DATA_LINE_LEN ? remaining_pad : HEX_DATA_LINE_LEN
      padded_line = ":" + curr_pad_length.to_hex_str + curr_memory_addr.to_hex_str + '00' + ('FF' * curr_pad_length)
      padded_line += calculate_crc(padded_line).to_hex_str
      combined_file_content << padded_line
      curr_memory_addr += curr_pad_length
	end
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
