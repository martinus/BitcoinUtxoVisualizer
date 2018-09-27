target_file = "../../out/all.blk"

source_files = Dir["../../out/0*.blk"].sort

# skip last one, it's not finished
source_files.pop

t = Time.now
File.open(target_file, "ab") do |output_stream|
	source_files.each do |source|
		File.open(source, "rb") do |source_stream|
			IO.copy_stream(source_stream, output_stream)
		end
		printf "."
	end
end
puts "done in #{Time.now - t} sec"
