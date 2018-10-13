require "./DateToBlock.rb"

require "json"
require "time"
require "pp"

=begin
from https://support.google.com/youtube/answer/2734698?hl=en&ref_topic=3014331
1
00:00:00,599 --> 00:00:04,160
>> ALICE: Hi, my name is Alice Miller and this is John Brown

2
00:00:04,160 --> 00:00:06,770
>> JOHN: and we're the owners of Miller Bakery.

3
00:00:06,770 --> 00:00:10,880
>> ALICE: Today we'll be teaching you how to make
our famous chocolate chip cookies!

4
00:00:10,880 --> 00:00:16,700
[intro music]

5
00:00:16,700 --> 00:00:21,480
Okay, so we have all the ingredients laid out here
=end


dtb = DateToBlock.load("../../out/headers.tsv")

# load all data from input directory
events = []
Dir["input/**/*.json"].each do |f|
    JSON.parse(File.read(f))["events"].each do |e|
        timestamp = Time.parse(e["date"]).to_i
        event = e["event"]

        events << [timestamp, event]
    end
end

events.sort!
events.each do |date, event|
    height, before, after = dtb.date_to_block(date)
    next unless before

    puts "block #{height}, #{Time.at(before)}: #{event}"
end

t = Time.parse("10th August 2013").to_i

result = dtb.date_to_block(t)
puts "search date: #{t}"
