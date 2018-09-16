require "pp"
require "./colormaps.rb"
require "./density.rb"

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




t = Time.now
density_filename = "density_3840x2160.bin"
utxo_filename = "utxo.bin"
density = Density.load(density_filename) || Density.new(3840, 2160, 0.00000001, 100000, 0, 550000)
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

colorized = density.colorize(ColorMaps.viridis)
printf "%6.3f: colorized\n", Time.now - t; t = Time.now

File.open("out.raw", "wb") do |fout|
	fout.write(colorized)
end
printf "%6.3f: file saved\n", Time.now - t; t=Time.now