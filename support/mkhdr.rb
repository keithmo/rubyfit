#!/usr/bin/env ruby

require 'csv'

STDOUT.sync = true 


if ARGV.length != 1
    puts "use: #{$0} file"
    exit 1
end

csv = CSV.read(ARGV[0])

in_enum = false
comma = ""
base_name = ""
maps = []

csv.each do |line|
    if in_enum
        if line[2].nil? && line[3].nil?
            in_enum = false
            puts "\n};"
            puts ""
        else
            print "#{comma}    MAP(#{line[2].upcase}, \"#{line[2]}\")"
            comma = ",\n"
        end
    else
        if line[1] == 'enum' || line[0] == 'manufacturer' || line[0] == 'garmin_product' || line[0] == 'antplus_device_type' || line[0] == 'battery_status'
            base_name = line[0]
            in_enum = true
            comma = ""

            puts "#undef PREFIX"
            puts "#define PREFIX FIT_#{base_name.upcase}_"
            puts "ID_MAP map_#{base_name}[] = {"
            maps << "map_#{base_name}"
        end
    end
end

comma = ""

puts "ID_MAPS maps[] = {"
maps.each do |m|
    print "#{comma}    { #{m}, DIM(#{m}) }"
    comma = ",\n"
end
puts "\n};"
