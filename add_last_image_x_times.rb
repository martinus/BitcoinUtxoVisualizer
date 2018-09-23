require "fileutils"

# copy last image so we get 10 seconds.
source_file = "img/00018072.png"
target_dir = "img_with_legend/"

fps = 60
seconds = 10

idx = File.basename(source_file).to_i
(fps * seconds).times do
	idx += 1	
	target_file = sprintf("%s%08d.png", target_dir, idx)
	raise "file #{target_file} already exists! doing nothing" if File.exist?(target_file) 
	FileUtils.cp(source_file, target_file)
end
