target_file = "../../out/all.blk"

source_files = Dir["../../out/blocks/0*.blk"].sort

# skip last one, it's not finished
#source_files.pop

if source_files.empty?
	puts "Could not find any file to add, quitting"
	exit(1)
end

t = Time.now

puts "creating #{target_file}"
File.open(target_file, "wb") do |output_stream|
	source_files.each do |source|
		puts "appending #{source}"
		File.open(source, "rb") do |source_stream|
			IO.copy_stream(source_stream, output_stream)
		end
	end
end
puts "done in #{Time.now - t} sec"
