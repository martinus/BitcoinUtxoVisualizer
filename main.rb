require "./blockreader.rb"
require "./utxo.rb"
require "./density.rb"
require "./colormaps.rb"

require 'thread' # for Queue

if $0 == __FILE__
	br = BlockReader.new('http://127.0.0.1:8332')
	filename = "utxo.bin"
	interval_save_seconds = 10*60
	interval_status_seconds = 10
	deadline_status = Time.now + interval_status_seconds
	deadline_save = Time.now + interval_save_seconds
	plot_interval = 30

	utxo = Utxo.load("utxo.bin") || Utxo.new
	
	queue = Queue.new
	
	fetcher = Thread.new(utxo.nextblockhash) do |nextblockhash|
		loop do
			block_data = br.block(nextblockhash)
			# "nextblockhash":"00000000000000000f23b502cee7b3d8c7931b755455a6b29eccd5de911e9fd7"}
			str = "\"nextblockhash\":\""
			found_idx = block_data.rindex(str)
			if found_idx.nil?
				queue.push nil
				break
			end
			nextblockhash = block_data.slice(found_idx + str.size, 64)
			
			# block_data not needed any more: push it to utxo worker
			queue.push(block_data)
			
			while queue.size > 100
				puts "queuesize > 100, waiting a second"
				sleep 1
			end
		end
	end
	
	density = Density.new(3840, 2160, 0.00000001, 100000, 0, 550000)
	loop do
		block_data = queue.pop
		if (block_data.nil?)
			puts "got nothing, saving and closing"
			utxo.save(filename)
			break
		end
		
		utxo.integrate_block_data(block_data)
		
		if (utxo.height % plot_interval == 0)
			density.clear
			utxo.data.each_value do |unspent_source_outputs|
				idx = 2
				# [block_height, voutnr_0, value_0, voutnr_1, value_1, ..., voutnr_n, value_n]
				block_height = unspent_source_outputs[0]
				
				while idx < unspent_source_outputs.size
					amount = unspent_source_outputs[idx]
					if amount > 0
						density.add(block_height, amount)
					end
					idx += 2				
				end
			end
			
			#density.save("#{utxo.height}.density.bin")
			colorized = density.colorize(ColorMaps.viridis, 1000)
			tmp_ppm_filename = "img/tmp.ppm"
			File.open(tmp_ppm_filename, "wb") do |fout|
				fout.printf("P6\n%d %d\n255\n", density.width, density.height)
				fout.write(colorized)
			end
			img_name = sprintf("img/%05d.png", utxo.height/plot_interval)
			
			`magick convert #{tmp_ppm_filename} #{img_name}`
		end
		
		now = Time.now

		if now >= deadline_status
			utxo.print_status(queue.size)
			deadline_status = Time.now + interval_status_seconds
		end		
		
		if now >= deadline_save
			utxo.save(filename)
			deadline_save = Time.now + interval_save_seconds
		end
	end
end
