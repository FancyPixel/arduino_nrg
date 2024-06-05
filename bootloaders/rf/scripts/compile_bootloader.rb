#!/usr/bin/env ruby

### This script compiles the RF bootloader
require 'fileutils'

MSP430_BSL_GEM_VERSION='0.3.0'.freeze

unless system "gem list msp430_bsl -v #{MSP430_BSL_GEM_VERSION} -i --silent"
  puts "Installing msp430_bsl-#{MSP430_BSL_GEM_VERSION}"
  system "gem install msp430_bsl -v #{MSP430_BSL_GEM_VERSION}"
end

kind_of_combine = ARGV.shift
# source_files_path = ARGV.shift
project_name = ARGV.shift.split('.').first
bl_version = ARGV.shift.to_i
must_execute = kind_of_combine == 'boot_and_fw'

BOOT_PRODUCT_H_TEMPLATE = File.expand_path File.join(__dir__, '../', 'product_types', "#{project_name}.h")
BOOT_PRODUCT_H_FILE_PATH = File.expand_path File.join(__dir__, '../', 'product.h')

# Break if we've not enabled "Combine bootloader and Sketch" ARDUINO IDE's option
unless must_execute
  exit 0
end

# if bl_version argument given
unless bl_version.zero?
  # Replace product version
  product_h_template_content = File.read BOOT_PRODUCT_H_TEMPLATE
  product_h_template_content[/const uint8_t FIRMWARE_VERSION\[\] = { .* };/] = "const uint8_t FIRMWARE_VERSION[] = { #{bl_version >> 24 & 0xff}, #{bl_version >> 16 & 0xff}, #{bl_version >> 8 & 0xff}, #{bl_version & 0xff} };"
  f = File.open(BOOT_PRODUCT_H_TEMPLATE, 'w')
  f.write product_h_template_content
  f.close
end

puts "\n\nCompiling RF bootloader\n\n"

# Copy mote's specific product.h template to effective product.h
FileUtils.cp BOOT_PRODUCT_H_TEMPLATE, BOOT_PRODUCT_H_FILE_PATH

RECOMPILE_COMMAND = "cd #{__dir__}/../ && make clean && make"

# TODO: Force recompile
exit system("#{RECOMPILE_COMMAND}")

