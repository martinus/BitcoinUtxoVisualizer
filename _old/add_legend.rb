require 'thread'
require 'pp'

offset_x = 13
resolution_y_pixel = 3840
resolution_y_blocks = 550000
blocks_per_image = 30

#input_legend = "legend.pgm"
#input = "img/**.png"
outdir = "img_with_legend"

headers_file = "headers.bin"
headers = []

#File.open(headers_file, "rb") do |f|
	#headers = Marshal.load(f)
#end


pointsize = 16
point_offset = 6

# fill up queue
queue = Queue.new

def format_time(time_str)
	"#{time_str} (#{Time.at(time_str).strftime("%a, %d %B %Y %H:%M:%S")})"
end

bad_files = Queue.new
workers = []
8.times do |i|
	t = Thread.new(i) do
		loop do 
			cmd = queue.pop
			if cmd.nil?
				puts "#{i}: done!"
				queue.push nil
				break
			end
			if !system(cmd)
				STDERR.puts "#{i}: Error in '#{cmd}''"
				bad_files.push cmd
			end
			puts "#{i}: #{cmd}"
		end
	end

	workers.push t
end


Dir["img/**.png"].each do |input_filename|
	basename = File.basename(input_filename)
	outfile = "#{outdir}/#{basename}"
	next if File.exist?(outfile)

	block_height = basename.to_i * blocks_per_image
	
	pos_line = resolution_y_pixel * block_height / resolution_y_blocks
	offset_composite_x = pos_line + offset_x

	# composit drawing
	cmd = "magick #{input_filename}"
	# set up annotations
	cmd += " -pointsize #{pointsize} -fill snow3"

	cmd += " -annotate +#{offset_composite_x+16}+#{0+pointsize} \"100 kBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{166+point_offset} \"10 kBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{332+point_offset} \"1 kBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{498+point_offset} \"100 BTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{664+point_offset} \"10 BTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{830+point_offset} \"1 BTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{996+point_offset} \"100 mBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{1163+point_offset} \"10 mBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{1329+point_offset} \"1 mBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{1495+point_offset} \"100 µBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{1661+point_offset} \"10 µBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{1827+point_offset} \"1 µBTC\""
	cmd += " -annotate +#{offset_composite_x+16}+#{1993+point_offset} \"1 Finney\""
	cmd += " -annotate +#{offset_composite_x+16}+#{2159-2} \"1 Satoshi\""
	
	blk = headers[block_height]
	off_y = 1900
	line_offset = 22
	off_y -= line_offset

	off_x = offset_composite_x + 170
	if offset_composite_x > 700
		off_x = 20
	end

	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"hash: #{blk["hash"]}\""	
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"height: #{blk["height"]}\""	
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"version: 0x#{blk["versionHex"]}\""
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"time: #{format_time(blk["time"])}\""
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"mediantime: #{format_time(blk["mediantime"])}\""
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"nonce: #{blk["nonce"]}\""
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"bits: #{blk["bits"]}\""
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"difficulty: #{blk["difficulty"]}\""
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"chainwork: #{blk["chainwork"]}\""
	#cmd += " -annotate +#{off_x}+#{off_y += line_offset} \"nTx: #{blk["nTx"]}\""

	# final output
	cmd += " -quality 10 #{outfile}"

	# magick img/00017674.png legend.pgm -geometry +3714+0 -quality 10 -composite -pointsize 15 -fill white -annotate +3730+15 "100 kBTC" -quality 100 out.png
	system(cmd)
	#queue.push cmd
end
#queue.push nil

