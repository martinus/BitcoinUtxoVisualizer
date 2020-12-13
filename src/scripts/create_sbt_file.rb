require "./DateToBlock.rb"

require "json"
require "time"
require "pp"

# from from https://support.google.com/youtube/answer/2734698?hl=en&ref_topic=3014331

=begin
0:00:00.599,0:00:04.160
>> ALICE: Hi, my name is Alice Miller and this is John Brown

0:00:04.160,0:00:06.770
>> JOHN: and we're the owners of Miller Bakery.

0:00:06.770,0:00:10.880
>> ALICE: Today we'll be teaching you how to make
our famous chocolate chip cookies!

0:00:10.880,0:00:16.700
[intro music]

0:00:16.700,0:00:21.480
Okay, so we have all the ingredients laid out here=end
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

# e.g. 10.88 -> 0:00:10.880
def sbv_time(t)
    i = t.to_i
    "%d:%02d:%02d.%03d" % [i/3600, i/60%60, i%60, (t*1000).to_i%1000]
end

events.sort!

visible_duration = 20.0
sbv_num = 0
events.each do |date, event|
    height, before, after = dtb.date_to_block(date)
    next unless before

    sbv_num += 1
    t = height / 60.0

    puts "#{sbv_time(t)},#{sbv_time(t + visible_duration)}"
    puts "#{Time.at(before).strftime("%b %-d, %Y")}: #{event}"
    puts
end

STDERR.puts "#{events.size} events created"