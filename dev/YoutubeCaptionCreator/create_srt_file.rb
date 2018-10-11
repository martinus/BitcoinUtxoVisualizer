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

# parse block dates
dates = []
File.open("../../out/headers.tsv", "rt") do |f|
    f.each_line do |l|
        l = l.split
        height = l[1].to_i
        mediantime = Time.at(l[3].to_i)
        dates[height] = mediantime
    end
end
pp dates

