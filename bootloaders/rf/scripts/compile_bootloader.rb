### This script compiles the RF bootloader

require 'bundler/setup'
Bundler.require

must_execute = ARGV.shift == 'true'

# Break if we've not enabled "Combine bootloader and Sketch" ARDUINO IDE's option
unless must_execute
  exit 0
end

# Check if code changed. Recompile if needed
code_changed = !`git status --porcelain`.empty?
unless code_changed
  puts "Bootloader code did not change, NOT recompiling"
  exit 0
end

puts "\n\nCompiling RF bootloader\n\n"

source_files_path = ARGV.shift

class Numeric
  def to_hex_str
    n = to_s(16).upcase
    if n.length.odd?
      n = "0#{n}"
    end
    n
  end
end

BOOT_GWAP_PRODUCT_CODE_TEMPLATE = "const uint8_t GWAP_PRODUCT_CODE[] = { %s };"
BOOT_PRODUCT_H_FILE_PATH = File.expand_path File.join(__dir__, '../', 'product.h')
SKETCH_PRODUCT_H_FILE_PATH = File.expand_path File.join(source_files_path, 'product.h')

RECOMPILE_COMMAND = "cd #{__dir__}/../ && make clean && make"

# Check product code
boot_product_code_file_content = File.read BOOT_PRODUCT_H_FILE_PATH
boot_match_lines = boot_product_code_file_content.lines.select { |line| line.include? 'GWAP_PRODUCT_CODE' }
# Break if number of matching lines is not OK
unless boot_match_lines.size == 1
  puts "Wrong number of matching lines in BOOTLOADER product.h".colorize :red
  exit -1
end

# Calculate bootloader PCODE
bootloader_pcode = boot_match_lines.first.scan(/\d+(?=,|\s)/).map!{ |n| n.to_i }.pack('C*').unpack('l>*').first

# Calculate sketch PCODE
sketch_product_code_file_content = File.read SKETCH_PRODUCT_H_FILE_PATH
sketch_match_lines = sketch_product_code_file_content.lines.select { |line| line.include? 'GWAP_PRODUCT_CODE' }
# Break if number of matching lines is not OK
unless sketch_match_lines.size == 1
  puts "Wrong number of matching lines in SKETCH product.h".colorize :red
  exit -1
end

sketch_pcode = sketch_match_lines.first.scan(/(?<!\/\/[\s*])\d+/).first.to_i

# TODO: Force recompile
exit system("#{RECOMPILE_COMMAND}")

=begin
	# If bootloader's PCODE is different from sketch' PCODE, recompile bootloader with correct PCODE
	if sketch_pcode != bootloader_pcode
	  puts "Fixing BOOTLOADER's PCODE and recompiling it"
	  # Copy correct PCODE into place
	  sketch_pcode_to_byte_array = [sketch_pcode].pack("l>*").unpack('C*')
	  boot_product_code_file_content[boot_match_lines.first.strip] = BOOT_GWAP_PRODUCT_CODE_TEMPLATE % sketch_pcode_to_byte_array.join(", ")
	  f = File.open(BOOT_PRODUCT_H_FILE_PATH, 'w')
	  f.write boot_product_code_file_content
	  f.close

	  system "#{RECOMPILE_COMMAND}"
	end
=end
