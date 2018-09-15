require "pp"

class Utxo
	attr_reader :nextblockhash, :height, :hash, :time
	
	def initialize	
		@utxo = {}
		# initialize with genesis block
		@nextblockhash = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"
		@height = nil
		@hash = nil
		@time = nil
	end
	
	def remove(vout, unspent_source_outputs)
		idx = 1
		while idx < unspent_source_outputs.size
			if vout == unspent_source_outputs[idx]
				# replace with last 2 elements
				unspent_source_outputs[idx] = unspent_source_outputs[-2]
				unspent_source_outputs[idx+1] = unspent_source_outputs[-1]
				unspent_source_outputs.pop
				unspent_source_outputs.pop
				return
			end
			idx += 2
		end
		
		raise "vout could not be found, something wrong!"
	end
	
	def process(block_data_unparsed)
		block_data = JSON.parse(block_data_unparsed)
		@height = block_data["height"]
		@hash = block_data["hash"]
		@time = block_data["time"]
	
		block_data["tx"].each_with_index do |tx, tx_idx|
			# remove all inputs consumed by this transaction from utxo
			if tx_idx != 0
				# first transaction is coinbase, has no inputs
				tx["vin"].each_with_index do |vin, i|
					source_txid = uid(vin["txid"])
					source_vout = vin["vout"]
					unspent_source_outputs = @utxo[source_txid]
					
					remove(source_vout, unspent_source_outputs)
					
					# if only block_height is left and no more output, remove the whole transaction
					@utxo.delete(source_txid) if 1 == unspent_source_outputs.size
				end
			end
			
			# add all outputs from this transaction to the utxo
			data = [height]
			tx["vout"].each_with_index do |vout, i|
				data.push(i)
				data.push(vout["value"])
			end	
			@utxo[uid(tx["txid"])] = data
			@nextblockhash = block_data["nextblockhash"]
		end
	end	

	# convert the hex txid into binary, and only uses the first 16 bytes.
	# This should be enough to prevent any collision.
	def uid(txid)
		[txid[0..32]].pack("H*")
	end	
	
	def plot(queue_size)
		start_time = Time.now
		# PP.pp(utxo, $>, 200)
		num_outputs = 0
=begin		
		@utxo.each_value do |val|
			num_outputs += (val.size - 1) / 2
		end
=end		
		t = Time.at(@time).to_datetime
		puts "#{@height}: #{t}. #{@utxo.size} tx in utxo, #{num_outputs} outputs. Block hash: #{@hash}. queue size: #{queue_size} #{Time.now-start_time}"
	end
	
	def data
		@utxo
	end
	
	def convert
		@utxo.each_value do |val|
			# val contains [block_idx, {id => value}]
			val[1] = val[1].to_a.flatten
			val.flatten!
		end
	end
end



class Density
	attr_reader :num_points
	
	def self.load(filename)
		if File.file?(filename)
			File.open(filename, "rb") do |f|
				return Marshal.load(f)
			end
		end
		nil
	end

	def initialize(width, height, min_btc, max_btc, min_blockid, max_blockid)
		# tries to load and if it fails, creates new
		@width = width
		@height = height
		@data = Array.new(@width*@height, 0)
	
		@k_btc, @d_btc = calc_k_d(Math.log(min_btc), 0,	Math.log(max_btc),	@height)
		@k_block, @d_block = calc_k_d(min_blockid, 0, max_blockid, @width)
	
		@colormap = colormap_jet
		@num_points = 0
	end
	
	def save(filename)
		File.open(filename, "wb") do |f|
			Marshal.dump(self, f)
		end
	end
	
	def calc_k_d(x1, y1, x2, y2)
		k = (y2.to_f - y1) / (x2 - x1)
		d = y1 - k*x1
		[k, d]
	end
	
	def truncate(min, val, max)
		if val < min
			min
		elsif val > max
			max
		else
			val
		end
	end
	
	def colormap_jet
		rgb = []
		256.times do |i|
			n = 4.0*i / 256
			r = truncate(0, 255 * [n-1.5,-n+4.5].min, 255).to_i
			g = truncate(0, 255 * [n-0.5,-n+3.5].min, 255).to_i
			b = truncate(0, 255 * [n+0.5,-n+2.5].min, 255).to_i
			
			rgb.push [r, g, b].pack("CCC")
		end
		rgb
	end	
	
	def block_amount_to_pixel(block_height, btc)
		pixel_x = @k_block * block_height + @d_block
		pixel_y = @height - (@k_btc * Math.log(btc) + @d_btc)
		
		#p [block_height, btc, pixel_x, pixel_y]
		[truncate(0, pixel_x.to_i, @width-1), truncate(0, pixel_y.to_i, @height-1)]
	end
	
	def add(block_height, btc)
		x, y = block_amount_to_pixel(block_height, btc)
		idx = y * @width + x
		@data[idx] += 1
		@num_points += 1
	end
	
	def colorize
		background = [0, 0, 0].pack("CCC")
		#background = [255, 255, 255].pack("CCC")
		
		x_min = 1e100
		x_max = 0
		data = @data.map do |x|
			if x == 0
				0
			else				
				# scale
				x /= 10.0
				
				# sigmoid
				#x = 256 * Math.tanh(x)
				x = 300 * x / (1+x)
				#x = x/((1 + x*x)**0.5)
				
				x_min = [x_min, x].min
				x_max = [x_max, x].max
				x
			end
		end
		
		puts "min=#{x_min}, max=#{x_max}"
		data = data.map do |x|
			if x == 0
				background
			else
				# truncate & 255
				x = truncate(0, (x - x_min), 255).to_i
				@colormap[x]
			end
		end
		
		data.join
	end
end


t = Time.now
density_filename = "density.bin"
utxo_filename = "utxo.bin"
density = Density.load(density_filename) || Density.new(11000, 6188, 0.00000001, 100000, 0, 550000)
printf "%6.3f: density image initialized\n", Time.now - t; t = Time.now

if 0 == density.num_points
	utxo = Utxo.new
	if File.file?(utxo_filename)
		printf "%6.3f: loading #{utxo_filename}\n", Time.now - t; t = Time.now
		File.open(utxo_filename, "rb") do |f|
			utxo = Marshal.load(f)
		end
		printf "%6.3f: loading #{utxo_filename} done\n", Time.now - t; t = Time.now
		
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
		printf "%6.3f: integrated #{density.num_points} points\n", Time.now - t; t = Time.now
	end
	
	density.save(density_filename)
	printf "%6.3f: density data saved\n", Time.now - t; t = Time.now
end

colorized = density.colorize
printf "%6.3f: colorized\n", Time.now - t; t = Time.now

File.open("out.raw", "wb") do |fout|
	fout.write(colorized)
end
printf "%6.3f: file saved\n", Time.now - t; t=Time.now