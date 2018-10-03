# start with
# jruby -J-Xmx20000m main.rb 
# for good parallelism

# TODO: use 7680×4320 (4320p - youtube might support this already)

require "./blockreader.rb"
require "./utxo.rb"
require "./change_serializer.rb"

require 'thread' # for SizedQueue
require 'json'
require 'fileutils'

if $0 == __FILE__
	base_url = 'http://127.0.0.1:8332'

	dir = "../../out/blocks/"
	filename_pattern = "#{dir}%08d.blk"
	
	interval_save_seconds = 60*60
	
	puts "loading/creating utxo"
	utxo = Utxo.new
	utxo_filename = "#{dir}last.utxo"
	if File.exists?(utxo_filename)
		puts "GC.start"
		GC.start
		puts "done"
		utxo = Utxo.load(utxo_filename)
		puts "GC.start"
		GC.start
		puts "done"
	end
	puts "loading/creating done!"

	block_height = 0
	blk_filename = nil
	
	br = BlockReader.new(base_url)

	# does the bulk of the work: all the UTXO calculation stuff, and integrate the utxo data
	deadline_save = Time.now + interval_save_seconds
	fetch_count = 0
	nextblockhash = utxo.nextblockhash

	loop do
		# fetch block and see if we have good data
		block_data = JSON.parse(br.block(nextblockhash))

		fetch_count += 1
		if fetch_count % 100 == 0
			printf "\n#{block_data["height"]}"
		elsif fetch_count % 10 == 0
			printf "o"
		else
			printf "."
		end

		if block_data["nextblockhash"].nil?
			puts "fetcher: no nextblock found for #{nextblockhash}. Trying again in a second."
			sleep 1
			next
		end

		if blk_filename.nil?
			blk_filename = sprintf(filename_pattern, block_data["height"])
			# new filename, don't append to existine file but create a new one.
			p blk_filename
			FileUtils.rm_f(blk_filename)
		end

		cs = ChangeSerializer.new(blk_filename)
		cs.begin_block(block_data["height"])
		utxo.integrate_block_data(block_data) do |block_height, amount|
			cs.add(block_height, (amount * 100_000_000).round)
		end
		cs.end_block
		
		# if deadline reached, safe utxo and switch to a new filename
		if Time.now >= deadline_save
			puts "\tutox_to_change: saving #{utxo_filename}"
			utxo.save(utxo_filename)
			puts "\tutox_to_change: saving done. Loading to clean up memory"
			# set to nil, THEN call load, so we don't need duplicate memory
			utxo = nil
			GC.start
			utxo = Utxo.load(utxo_filename)
			puts "\tutox_to_change: loading done!"
			deadline_save = Time.now + interval_save_seconds
			blk_filename = nil
		end

		# got good data, integrate it
		nextblockhash = block_data["nextblockhash"]
	end
end
