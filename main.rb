# start with
# jruby -J-Xmx20000m main.rb 
# for good parallelism

# TODO: use 7680×4320 (4320p - youtube might support this already)

require "./blockreader.rb"
require "./utxo.rb"
require "./density.rb"
require "./colorize.rb"
require "./colormaps.rb"

require 'thread' # for Queue

if $0 == __FILE__
	br = BlockReader.new('http://127.0.0.1:8332')
	utxo_filename = "utxo.bin"
	
	interval_save_seconds = 30*60
	deadline_save = Time.now + interval_save_seconds
	
	plot_interval = 30

	puts "loading/creating utxo"
	utxo = Utxo.load(utxo_filename) || Utxo.new
	puts "loading/creating done!"
	
	queue_blockdata = Queue.new
	queue_imagedata = Queue.new
	
	# this thread fetches data from the bitcoin JSON service as fast as it can.
	# It does minimal processing just to find out the nextblockhash so that the next block
	# can be fetched immediately so bitcoind always has work to do.
	fetcher = Thread.new(utxo.nextblockhash) do |nextblockhash|
		block_count = 0
		loop do
			block_data = br.block(nextblockhash)
			block_count += 1
			if block_count % 100 == 0
				puts "fetcher: fetched 100 blocks. currently at #{nextblockhash}, queue_blockdata: #{queue_blockdata.size} entries"
			end
			
			# "nextblockhash":"00000000000000000f23b502cee7b3d8c7931b755455a6b29eccd5de911e9fd7"}
			str = "\"nextblockhash\":\""
			found_idx = block_data.rindex(str)
			if found_idx.nil?
				# no nextblock found: send nil as the sentinel and quit.
				queue_blockdata.push nil
				break
			end
			nextblockhash = block_data.slice(found_idx + str.size, 64)
			
			# block_data not needed any more: push it to utxo worker
			queue_blockdata.push(block_data)
			
			if queue_blockdata.size > 100
				puts "fetcher: waiting, queue full. queues: #{queue_blockdata.size} -> #{queue_imagedata.size}"
				while queue_blockdata.size > 50
					sleep 0.1
				end
				puts "fetcher: continue fetching. queues: #{queue_blockdata.size} -> #{queue_imagedata.size}"
			end
		end
	end
	
	# this thread is currently the bottleneck. It does all the UTXO calculation stuff.
	# I need to profile it somehow.
	# TODO make it faster
	utxo_imagecalc = Thread.new do
		density = Density.new(3840, 2160, 0.00000001, 100000, 0, 550000)
		
		bench_integrate = 0.0
		bench_density = 0.0
		
		loop do
			block_data = queue_blockdata.pop
			if (block_data.nil?)
				puts "\tutxo_imagecalc: got sentinel, saving and ending."
				utxo.save(utxo_filename)
				queue_imagedata.push nil
				break
			end
			
			# Integrating's performance is ok. At least it's only O(size of current block).
			# postprocessing could be sped up by integrating density processing into utxo
			# Also, I should rewrite it in Rust...
			t = Time.now
			utxo.integrate_block_data(block_data)
			bench_integrate += Time.now - t
			
			if (utxo.height % plot_interval == 0)
				density.clear

				# generate density data. This is slow as it iterats over *all* unspent transaction outputs.
				t = Time.now
				utxo.data.each_value do |unspent_source_outputs|
					# content of unspent_source_outputs: 
					# [block_height, voutnr_0, amount_0, voutnr_1, amount_1, ..., voutnr_n, amount_n]					
					block_height = unspent_source_outputs[0]

					# iterate all amounts
					idx = 2
					while idx < unspent_source_outputs.size
						amount = unspent_source_outputs[idx]
						density.add(block_height, amount) if amount > 0
						idx += 2				
					end
				end
				bench_density += Time.now - t
				
				#density.save("#{utxo.height}.density.bin")
				#utxo.print_status(queue_blockdata.size)				
				queue_imagedata.push [utxo.height, density.data.clone, density.width, density.height]
				puts "\tutxo_imagecalc: density utxo at block height #{utxo.height}. bench: #{bench_integrate} sec integrate, #{bench_density} sec density. queues: #{queue_blockdata.size} -> #{queue_imagedata.size}"
				
				while queue_imagedata.size > 100
					puts "\tfetcher: waiting, queue full. queues: #{queue_blockdata.size} -> #{queue_imagedata.size}"
					sleep 1
				end				
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
	
	# write output image from density map
	loop do
		block_height, density_data, width, height = queue_imagedata.pop
		break if density_data.nil?
		
		colorized = Colorize.colorize(density_data, ColorMaps.viridis, 1000)			
		
		tmp_ppm_filename = "img/tmp.ppm"
		File.open(tmp_ppm_filename, "wb") do |fout|
			fout.printf("P6\n%d %d\n255\n", width, height)
			fout.write(colorized)
		end
		img_name = sprintf("img/%08d.png", block_height/plot_interval)
		
		system("magick convert #{tmp_ppm_filename} #{img_name}")
		puts "\t\timage_creator: wrote #{img_name}"
	end
end
