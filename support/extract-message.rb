#!/usr/bin/env ruby

require 'csv'

STDOUT.sync = true 


if ARGV.length != 1
    puts "use: #{$0} file"
    exit 1
end

csv = CSV.read(ARGV[0])

in_enum = false
base_name = ""
fields = {}

csv.each do |line|
    if in_enum
        if line[0].nil?
            if line[1].nil?
                in_enum = false
            else
                name = "FIT_#{base_name}_FIELD_NUM_#{line[2].upcase}"
                value =  "#{line[1]}"

                puts "    #{name} = #{value}"
                fields[base_name] << { sym: name, short: line[2] }
            end
        end
    else
        if !line[0].nil?  && line[1].nil?
            base_name = line[0].upcase
            in_enum = true
            fields[base_name] = []
        end
    end
end

puts "    @Fit_Fields = {"
fields.keys.each do |k|
    puts "        'FIT_MESG_NUM_#{k}' => {"
    fields[k].each do |f|
        puts "            #{f[:sym]} => '#{f[:short]}',"
    end
    puts "        },"
end
puts "    }"
