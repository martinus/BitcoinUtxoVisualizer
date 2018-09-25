# start with
# jruby -J-Xmx20000m main.rb 
# for good parallelism

# TODO: use 7680×4320 (4320p - youtube might support this already)

require "./blockreader.rb"
require "./utxo.rb"
require "./change_serializer.rb"

require 'thread' # for SizedQueue
require 'json'

if $0 == __FILE__
	base_url = 'http://127.0.0.1:8332'
	
	utxo_filename = '../../out/utxo.bin'
	interval_save_seconds = 30*60
	
	puts "loading/creating utxo"
	utxo = Utxo.load(utxo_filename) || Utxo.new
	puts "loading/creating done!"
	
	# sized queues block when they are full
	queue_blockdata = SizedQueue.new(20)
	queue_jsondata = SizedQueue.new(20)
	
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
	deadline_save = Time.now + interval_save_seconds

	loop do
		block_data = queue_jsondata.pop
		if (block_data.nil?)
			puts "\tutox_to_change: got sentinel, saving and ending."
			utxo.save(utxo_filename)
			break
		end

		filename = sprintf("../../out/change_%08d.bin", (block_data["height"] / 1000)*1000)
		cs = ChangeSerializer.new(filename)
		
		# count: 1 for add, -1 for remove
		cs.begin_block(block_data["height"])
		utxo.integrate_block_data(block_data) do |count, block_height, amount|
			cs.add(block_height, (amount * 100_000_000).to_i, count > 0)
		end
		cs.end_block
		
		# deadline reached, save the current state
		if Time.now >= deadline_save
			puts "\tutox_to_change: saving #{utxo_filename}"
			utxo.save(utxo_filename)
			puts "\tutox_to_change: saving done"
			deadline_save = Time.now + interval_save_seconds
		end
	end
end
