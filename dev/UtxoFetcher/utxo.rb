require "./blockreader.rb"
require 'pp'
require 'date'

# Stores the current Utxo data set. Data structure
#   txid: 16byte prefix (truncated to 128bit, which should be enough to prevent collisions)
#   data: [block_height, (voutnr_0, amount_0), (voutnr_1, amount_1), ..., (voutnr_n, amount_n)]
#         blockheight, and all the vout values still present in the blockchain. Will be removed by vin's.
#   utxo: unordered_map<txid, data>
#
# So basically we need a map from txid to a map of voutnr to amount. Memory is an issue, so make it very compact:
#   using TxId = std::array<uint8_t, 16>;  // 128 bit should be enough to prevent any collisions
#   using VOutNr = uint16_t;               // maximum 65k outputs is enough (record is ~13k)
#   using Satoshi = std::array<uint8_t, 8> // use array so we have no padding. Must use std::memcpy.
# 
#   std::unordered_flat_map<Txid, std::unordered_flat_map<VOutNr, Satoshi>>
#      // flat_maps should be fine, but maybe experiment with different maps
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
	
	def self.load(filename)
		if File.file?(filename)
			File.open(filename, "rb") do |f|
				return Marshal.load(f)
			end
		end
		nil
	end

	def save(filename)
		tmp_name = filename + ".tmp"
		File.open(tmp_name, "wb") do |f|
			Marshal.dump(self, f)
		end
		File.delete(filename) if File.exists?(filename)
		File.rename(tmp_name, filename)		
	end

	# iterates all unspent outputs
	def each(&vout_vin_handler)
		@utxo.each_value do |unspent_source_outputs|
			# content of unspent_source_outputs: 
			# [block_height, voutnr_0, amount_0, voutnr_1, amount_1, ..., voutnr_n, amount_n]					
			block_height = unspent_source_outputs[0]

			# iterate all amounts
			idx = 2
			while idx < unspent_source_outputs.size
				amount = unspent_source_outputs[idx]
				vout_vin_handler.call(block_height, amount)
				idx += 2				
			end
		end
	end	
	
	def remove(vout, unspent_source_outputs, vout_vin_handler)
		idx = 1
		while idx < unspent_source_outputs.size
			if vout == unspent_source_outputs[idx]
				# [block_height, voutnr_0, amount_0, voutnr_1, amount_1, ..., voutnr_n, amount_n]
				block_height = unspent_source_outputs[0]
				amount = unspent_source_outputs[idx + 1]
				vout_vin_handler.call(block_height, -amount)

				# replace with last 2 elements
				unspent_source_outputs[idx] = unspent_source_outputs[-2]
				unspent_source_outputs[idx+1] = unspent_source_outputs[-1]
				unspent_source_outputs.pop(2)
				return
			end
			idx += 2
		end
		
		raise "vout could not be found, something wrong!"
	end
	
	# This should be fast
	def integrate_block_data(block_data, &vout_vin_handler)
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
					
					remove(source_vout, unspent_source_outputs, vout_vin_handler)
					
					# if only block_height is left and no more output, remove the whole transaction
					@utxo.delete(source_txid) if 1 == unspent_source_outputs.size
				end
			end
			
			# add all outputs from this transaction to the utxo
			data = [height]
			tx["vout"].each_with_index do |vout, i|
				data.push(i)
				
				val = vout["value"]
				data.push(val)

				# count is 1 to add this
				vout_vin_handler.call(height, val)
				
				# val can be 0, that's ok. See e.g. tx 2f2442f68e38b980a6c4cec21e71851b0d8a5847d85208331a27321a9967bbd6
				raise "tx error: tx=#{tx}, i=#{i}, vout=#{vout["value"]}" if val < 0 || i < 0
			end	
			@utxo[uid(tx["txid"])] = data
			@nextblockhash = block_data["nextblockhash"]
		end
	end
	
	def data
		@utxo
	end	

	# convert the hex txid into binary, and only uses the first 16 bytes.
	# This should be enough to prevent any collision.
	def uid(txid)
		[txid[0..32]].pack("H*")
	end	
	
	def print_status(queue_size)
		start_time = Time.now
		# PP.pp(utxo, $>, 200)
		t = Time.at(@time).to_datetime
		puts "#{@height}: #{t}. #{@utxo.size} tx in utxo. Block hash: #{@hash}. queue size: #{queue_size} #{Time.now-start_time}"
	end
end

