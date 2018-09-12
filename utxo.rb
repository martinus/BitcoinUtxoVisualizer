require 'open-uri'
require 'net/http'
require 'uri'
require 'json'
require 'pp'
require 'thread'

class BlockReader
	def initialize(uri)
		@uri = uri
	end
	
	def read(what)	
		begin
			uri = @uri + '/rest/' + what + '.json'
			open(uri).read
		rescue
			STDERR.puts "something went wrong. Trying again in a second"
			sleep 1.0
			retry 
		end
	end
	
	def chaininfo
		read('chaininfo')
	end
	
	def block(hash)
		read("block/#{hash}")
	end	
end

class Utxo
	attr_reader :nextblockhash, :height, :hash
	
	def initialize	
		@utxo = {}
		# initialize with genesis block
		@nextblockhash = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"
		@height = nil
		@hash = nil
	end
	
	def process(block_data_unparsed)
		block_data = JSON.parse(block_data_unparsed)
		@height = block_data["height"]
		@hash = block_data["hash"]
	
		block_data["tx"].each_with_index do |tx, tx_idx|
			# remove all inputs consumed by this transaction from utxo
			if tx_idx != 0
				# first transaction is coinbase, has no inputs
				tx["vin"].each_with_index do |vin, i|
					source_txid = uid(vin["txid"])
					source_vout = vin["vout"]
					unspent_source_outputs = @utxo[source_txid][1]
					if unspent_source_outputs.delete(source_vout).nil?
						puts "ERROR: txid #{tx["hash"]}: removing {source_txid: #{vin["txid"]}, source_vout: #{source_vout}} but already gone"
						pp(@utxo[source_txid], $>, 200)
						puts
						puts
					end
					@utxo.delete(source_txid) if unspent_source_outputs.empty?
				end
			end
			
			# add all outputs from this transaction to the utxo
			outputs = {}
			tx["vout"].each_with_index do |vout, i|
				outputs[i] = vout["value"]				
			end	
			@utxo[uid(tx["txid"])] = [@height, outputs]
			@nextblockhash = block_data["nextblockhash"]
		end
	end	

	# convert the hex txid into binary, and only uses the first 16 bytes.
	# This should be enough to prevent any collision.
	def uid(txid)
		[txid[0..32]].pack("H*")
	end	
	
	def plot
		# PP.pp(utxo, $>, 200)
		num_outputs = 0
		@utxo.each do |key, val|
			num_outputs += val.size
		end
		puts "#{@height}: #{@utxo.size} tx in utxo, #{num_outputs} outputs. Block hash: #{@hash}"
	end
end



if $0 == __FILE__
	br = BlockReader.new('http://127.0.0.1:8332')
	filename = "utxo.bin"
	interval_save_seconds = 60
	interval_status_seconds = 2

	utxo = Utxo.new
	if File.file?(filename)
		utxo = Marshal.load(File.binread(filename))
	end
	
	deadline_save = Time.now + interval_save_seconds
	deadline_status = Time.now + interval_status_seconds;
	
	loop do
		block_data = br.block(utxo.nextblockhash)
		utxo.process(block_data)

		now = Time.now
		
		if now >= deadline_status
			utxo.plot
			deadline_status += interval_status_seconds
		end
		
		
		if now >= deadline_save
			puts "writing utxo at #{utxo.height}"
			File.open(filename, "wb") do |f|
				f.write(Marshal.dump(utxo))
			end
			deadline_save += interval_save_seconds
		end
	end
end
