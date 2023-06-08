require 'bundler/setup'
Bundler.require

UROM_START = 0x8000
UROM_END = 0xFDFF
FLASH_SEGMENT_SIZE = 0x200  # 512 bytes

HEX_FILE_PATH = File.expand_path File.join(__dir__, '../', 'rfloader.hex')
ELF_FILE_PATH = File.expand_path File.join(__dir__, '../', 'rfloader.elf')
CODESIZE_FILE_PATH = File.expand_path File.join(__dir__, '../', 'codesize.h')
MEMORY_RF_FILE_PATH = File.expand_path File.join(__dir__, '../../../', 'ldscript/memory_rf.x')
BOARDS_FILE_PATH = File.expand_path File.join(__dir__, '../../../', 'boards.txt')

PROGRAM_DATA_SIZE_REGEX = /^(?:\.text|\.data|\.bootloader)\s+([0-9]+).*/
MEMORY_SIZE_REGEX = /^(?:\.data|\.bss|\.noinit)\s+([0-9]+).*/
CODESIZE_REGEX =/BOOTLOADER_CODE_SIZE\s(\d+)/
MEMORY_RF_UROM_REGEX = /^\s*urom\s\(rx\).+$/
BOARDS_NRG2_MAX_SIZE_REGEX = /^nrg2.upload.maximum_size=\d+$/

class Numeric
  def to_hex_str
    n = to_s(16).upcase
    if n.length.odd?
      n = "0#{n}"
    end
    n
  end
end

PROGRAM_SIZE_COMMAND = "~/Library/Arduino15/packages/panstamp_nrg/tools/msp430-gcc/4.6.3/bin/msp430-size -A %s"
RECOMPILE_COMMAND = "make clean && make"
MEMORY_RF_UROM_LINE_TEMPLATE = "  urom (rx)        : ORIGIN = %s, LENGTH = %s /* END=0xFDFF, size %s */"
BOARDS_NRG2_UPLOAD_MAX_SIZE = "nrg2.upload.maximum_size=%s"

### PRINT VECTOR TABLE
hex_file_content = File.read HEX_FILE_PATH

data = []
index = 0

hex_file_content.each_line do |line|
  if line.start_with?(":10FF")
  	line.strip!
	line = line[9..-3]
	# uint8_t vector_1[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	string = "uint8_t vector_#{index}[] = { "
	line.scan(/.{2}/).each { |byte| string += "0x#{byte}, " }
	data << (string[0..-3] + ' };')
	index += 1
  end
end

puts "\n\n ***\e[31m  COPY AND SUBSTITUTE THESE VECTORS INTO YOUR factoryReset() FUNCTION \e[0m*** \n\n"
puts "\e[33m"
puts data
puts "\e[0m"


### PRINT BOOTLOADER SIZE
program_size_data = `#{ PROGRAM_SIZE_COMMAND % ELF_FILE_PATH }`

program_data_size = program_size_data.scan(PROGRAM_DATA_SIZE_REGEX).flatten.map(&:to_i).reduce :+
memory_size = program_size_data.scan(MEMORY_SIZE_REGEX).flatten.map(&:to_i).reduce :+
urom_new_origin = ((UROM_START + program_data_size) / FLASH_SEGMENT_SIZE.to_f).ceil * FLASH_SEGMENT_SIZE
urom_length = UROM_END - urom_new_origin

puts "\n\n\e[32m *** PROGRAM INFO *** \e[0m\n\n"
puts "\e[33m - Bootloader size:\e[0m #{program_data_size} bytes\n\n"
puts "\e[33m - Concentrator's startFirmwareAddress:\e[0m #{urom_new_origin.to_hex_str}\n\n"


### IF NECESSARY, UPDATE >BOOTLOADER_CODE_SIZE<  #define and recompile

codesize_file_content = File.read CODESIZE_FILE_PATH
prev_bootloader_code_size = codesize_file_content.scan(CODESIZE_REGEX).flatten.first.to_i
# If bootloader size changed, update codesize file header and recompile
if prev_bootloader_code_size != program_data_size
  # Update needed files and recompile
  system "clear"
  puts "\n\n\e[32m *** Bootloader code modified, proceed with a new compilation *** \e[0m\n"

  # Update codesize.h
  puts " - Updating\e[33m codesize.h\e[0m \e[32mBOOTLOADER_CODE_SIZE #define\e[0m\n"
  # Replace old size with new size
  codesize_file_content[CODESIZE_REGEX] = "BOOTLOADER_CODE_SIZE #{program_data_size}"
  # Update codesize.h
  file = File.open(CODESIZE_FILE_PATH, 'w')
  file.write codesize_file_content
  file.close

  # Update ldscript/memory_rf.x
  puts " - Updating\e[33m memory_rf.x\e[0m \e[32murom entry\e[0m\n"

  memory_rf_file_content = File.read MEMORY_RF_FILE_PATH
  memory_rf_file_content[MEMORY_RF_UROM_REGEX] = MEMORY_RF_UROM_LINE_TEMPLATE % [ "0x#{urom_new_origin.to_hex_str}", "0x#{urom_length.to_hex_str}", urom_length ]

  file = File.open(MEMORY_RF_FILE_PATH, 'w')
  file.write memory_rf_file_content
  file.close

  # Update boards.txt
  puts " - Updating\e[33m boards.txt\e[0m \e[32nrg2.upload.maximum_size\e[0m\n"
  boards_file_content = File.read BOARDS_FILE_PATH
  boards_file_content[BOARDS_NRG2_MAX_SIZE_REGEX] = BOARDS_NRG2_UPLOAD_MAX_SIZE % [urom_length]
  file = File.open(BOARDS_FILE_PATH, 'w')
  file.write boards_file_content
  file.close

  # Recompile
  puts " - \e[33m Recompiling...\e[0m"
  system "#{RECOMPILE_COMMAND}"
end

