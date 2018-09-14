require 'net/http'
require 'uri'
require 'json'
require 'pp'
require 'thread' # for Queue
require 'date'

class BlockReader
	def initialize(uri)
		@uri = uri
	end
	
	def read(what)	
		begin
			# see https://jhawthorn.github.io/curl-to-ruby/
			uri = URI.parse(@uri + '/rest/' + what + '.json')
			Net::HTTP.get_response(uri).body
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
		# PP.pp(utxo, $>, 200)
		num_outputs = 0
		@utxo.each do |key, val|
			num_outputs += (val.size - 1) / 2
		end
		t = Time.at(@time).to_datetime
		puts "#{@height}: #{t}. #{@utxo.size} tx in utxo, #{num_outputs} outputs. Block hash: #{@hash}. queue size: #{queue_size}"
	end
	
	def convert
		@utxo.each_value do |val|
			# val contains [block_idx, {id => value}]
			val[1] = val[1].to_a.flatten
			val.flatten!
		end
	end
end


def save(utxo, filename)
	puts "writing utxo at #{utxo.height}"
	File.open(filename, "wb") do |f|
		Marshal.dump(utxo, f)
	end
	puts "writing done #{utxo.height}"
end

if $0 == __FILE__
	br = BlockReader.new('http://127.0.0.1:8332')
	filename = "utxo.bin"
	interval_save_seconds = 10*60
	interval_status_seconds = 2
	deadline_status = Time.now + interval_status_seconds
	deadline_save = Time.now + interval_save_seconds

	utxo = Utxo.new
	if File.file?(filename)
		t = Time.now
		puts "loading #{filename}"
		File.open(filename, "rb") do |f|
			utxo = Marshal.load(f)
		end
		puts "loading done! #{Time.now - t} sec"
	end
	
	
	queue = Queue.new
	
	fetcher = Thread.new(utxo.nextblockhash) do |nextblockhash|
		loop do
			block_data = br.block(nextblockhash)
			# "nextblockhash":"00000000000000000f23b502cee7b3d8c7931b755455a6b29eccd5de911e9fd7"}
			str = "\"nextblockhash\":\""
			found_idx = block_data.rindex(str)
			if found_idx.nil?
				queue.push nil
				return
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
	
	loop do
		block_data = queue.pop
		if (block_data.nil?)
			puts "got nothing, saving and closing"
			save(utxo, filename)
			return
		end
		
		utxo.process(block_data)

		now = Time.now
		
		if now >= deadline_status
			utxo.plot(queue.size)
			deadline_status = Time.now + interval_status_seconds
		end
		
		
		if now >= deadline_save
			save(utxo, filename)
			deadline_save = Time.now + interval_save_seconds
		end
	end
end
