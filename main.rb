# start with
# jruby -J-Xmx20000m main.rb 
# for good parallelism

# TODO: use 7680×4320 (4320p - youtube might support this already)

require "./blockreader.rb"
require "./utxo.rb"
require "./density.rb"
require "./colorize.rb"
require "./colormaps.rb"

require 'thread' # for SizedQueue
require 'json'

if $0 == __FILE__
	base_url = 'http://127.0.0.1:8332'
	
	utxo_filename = 'utxo.bin'
	interval_save_seconds = 30*60
	
	# create image every x blocks
	create_image_every_x_block = 30


	puts "loading/creating utxo"
	utxo = Utxo.load(utxo_filename) || Utxo.new
	puts "loading/creating done!"
	
	# sized queues block when they are full
	queue_blockdata = SizedQueue.new(20)
	queue_jsondata = SizedQueue.new(20)
	queue_imagedata = SizedQueue.new(20)
	
	# this thread fetches data from the bitcoin JSON service as fast as it can.
	# It does minimal processing just to find out the nextblockhash so that the next block
	# can be fetched immediately so bitcoind always has work to do.
	fetcher = Thread.new(utxo.nextblockhash) do |nextblockhash|
		br = BlockReader.new(base_url)

		fetch_count = 0
		loop do
			block_data = br.block(nextblockhash)
			fetch_count += 1
			if fetch_count % 100 == 0
				puts "fetcher: fetched 100 blocks. currently at #{nextblockhash}, queue_blockdata: #{queue_blockdata.size} entries"
			end
			
			# "nextblockhash":"00000000000000000f23b502cee7b3d8c7931b755455a6b29eccd5de911e9fd7"}
			str = "\"nextblockhash\":\""
			found_idx = block_data.rindex(str)
			if found_idx.nil?
				puts "fetcher: no nextblock found for #{nextblockhash}. Trying again in a second."
				sleep 1
			else
				nextblockhash = block_data.slice(found_idx + str.size, 64)
				# block_data not needed any more: push it to utxo worker
				queue_blockdata.push(block_data)
			end
		end
	end

	# just parse the string into a JSON message
	jsonparser = Thread.new do
		loop do
			unparsed_block_data = queue_blockdata.pop
			if unparsed_block_data.nil?
				puts "jsonparser: got nil, forwarding nil"
				queue_jsondata.push(nil)
				break
			end
			parsed = JSON.parse(unparsed_block_data)
			queue_jsondata.push(parsed)
		end
	end
	
	# does the bulk of the work: all the UTXO calculation stuff, and integrate the utxo data
	# into a density image.
	utxo_imagecalc = Thread.new do
		deadline_save = Time.now + interval_save_seconds

		puts "\tutxo_imagecalc: initializing density with current utxo"
		density = Density.new(3840, 2160, 0.00000001, 100000, 0, 550000)
		utxo.each do |count, block_height, amount|
			density.modify(count, block_height, amount)
		end
		puts "\tutxo_imagecalc: density initialized"
		
		loop do
			block_data = queue_jsondata.pop
			if (block_data.nil?)
				puts "\tutxo_imagecalc: got sentinel, saving and ending."
				utxo.save(utxo_filename)
				queue_imagedata.push nil
				break
			end
			
			utxo.integrate_block_data(block_data) do |count, block_height, amount|
				density.modify(count, block_height, amount)
			end

			if (utxo.height % create_image_every_x_block == 0)
				queue_imagedata.push [utxo.height, density.data.clone, density.width, density.height]
			end
			
			# deadline reached, save the current state
			if Time.now >= deadline_save
				puts "\tutxo_imagecalc: saving #{utxo_filename}"
				utxo.save(utxo_filename)
				puts "\tutxo_imagecalc: saving done"
				deadline_save = Time.now + interval_save_seconds
			end
		end
	end
	
	image_converter = Thread.new do
		# write output image from density map
		colorizer = Colorize.new(ColorMaps.viridis, 1000)
		loop do
			block_height, density_data, width, height = queue_imagedata.pop
			break if density_data.nil?
			
			tmp_ppm_filename = "img/tmp.ppm"
			File.open(tmp_ppm_filename, "wb") do |fout|
				fout.printf("P6\n%d %d\n255\n", width, height)
				colorizer.colorize(density_data, fout)
			end
			img_name = sprintf("img/%08d.png", block_height/create_image_every_x_block)
			
			system("magick convert #{tmp_ppm_filename} #{img_name}")
			puts "\t\timage_creator: wrote #{img_name}"
		end
	end

	loop do
		break if !image_converter.alive?
		puts "Queues: block -> json -> image: #{queue_blockdata.size} -> #{queue_jsondata.size} -> #{queue_imagedata.size}"
		sleep 2
	end
end
