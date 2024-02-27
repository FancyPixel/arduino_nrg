#!/usr/bin/env ruby

stop_after_compilation = ENV['STOP_AFTER_COMPILATION'] == 'true'

MSP430_BSL_GEM_VERSION='0.3.0'.freeze

unless system "gem list msp430_bsl -v #{MSP430_BSL_GEM_VERSION} -i --silent"
  puts "Installing msp430_bsl-#{MSP430_BSL_GEM_VERSION}"
  system "gem install msp430_bsl -v #{MSP430_BSL_GEM_VERSION}"
end

UROM_START = 0x8000
# UROM_END = 0xFF80   # Matches VECTOR_TABLE_SEGMENT start
UROM_END = 0xFE00   # Matches VECTOR_TABLE_SEGMENT start
FLASH_SEGMENT_SIZE = 0x200  # 512 bytes

HEX_FILE_PATH = File.expand_path File.join(__dir__, '../', 'rfloader.hex')
ELF_FILE_PATH = File.expand_path File.join(__dir__, '../', 'rfloader.elf')
CODESIZE_FILE_PATH = File.expand_path File.join(__dir__, '../', 'codesize.h')
MEMORY_RF_FILE_PATH = File.expand_path File.join(__dir__, '../../../', 'ldscript/memory_rf.x')
BOARDS_FILE_PATH = File.expand_path File.join(__dir__, '../../../', 'boards.txt')
FFC0_VECTOR_HEX_FILE_REGEX = /^(:10FFC0.*)$/

BOOT_DATA_SIZE_REGEX = /^(?:\.text|\.data|.rodata|\.bootloader)\s+([0-9]+).*/
BOOT_MEMORY_SIZE_REGEX = /^(?:\.data|\.bss|\.noinit)\s+([0-9]+).*/
CODESIZE_REGEX =/BOOTLOADER_CODE_SIZE\s(\d+)/
CODESIZE_ISR_VECTOR_REGEX = /^.*isrVector_FFC0\[\].*$/
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
RECOMPILE_COMMAND = "make clean && STOP_AFTER_COMPILATION=true make"
MEMORY_RF_UROM_LINE_TEMPLATE = "  urom (rx)        : ORIGIN = %s, LENGTH = %s /* END=0xFDFF, size %s */"
BOARDS_NRG2_UPLOAD_MAX_SIZE = "nrg2.upload.maximum_size=%s"

### ADD VECTOR TABLE to codesize.h
hex_file_content = File.read HEX_FILE_PATH

### PRINT BOOTLOADER SIZE
boot_size_data = `#{ PROGRAM_SIZE_COMMAND % ELF_FILE_PATH }`

boot_program_size = boot_size_data.scan(BOOT_DATA_SIZE_REGEX).flatten.map(&:to_i).reduce :+
boot_memory_size = boot_size_data.scan(BOOT_MEMORY_SIZE_REGEX).flatten.map(&:to_i).reduce :+
urom_new_origin = ((UROM_START + boot_program_size) / FLASH_SEGMENT_SIZE.to_f).ceil * FLASH_SEGMENT_SIZE
urom_length = UROM_END - urom_new_origin

puts "\n\n\e[32m *** PROGRAM INFO *** \e[0m\n\n"
puts "\e[33m - Bootloader size:\e[0m 0x#{boot_program_size.to_hex_str} (#{boot_program_size}) bytes - Used memory: #{boot_memory_size} bytes\n\n"
puts "\e[33m - Concentrator's startFirmwareAddress:\e[0m 0x#{urom_new_origin.to_hex_str}\n\n"


### UPDATE codesize.h and recompile
# Force an update of codesize file header and recompile
unless stop_after_compilation
  # Update needed files and recompile
  system "clear"

  codesize_file_content = File.read CODESIZE_FILE_PATH

  # Update codesize.h
  puts " - Updating\e[33m codesize.h\e[0m \e[32mBOOTLOADER_CODE_SIZE #define\e[0m\n"
  # Replace old size with new size
  codesize_file_content[CODESIZE_REGEX] = "BOOTLOADER_CODE_SIZE #{boot_program_size}"
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

