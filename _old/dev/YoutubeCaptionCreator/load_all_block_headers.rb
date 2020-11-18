require '../UtxoFetcher/blockreader.rb'
require 'json'
require 'pp'

br = BlockReader.new('http://127.0.0.1:8332')

genesis_block = '000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f'

puts "hash\theight\ttime\tmediantime\tdifficulty\tnTx"
File.open("headers.tsv", "wt") do |f|
    block = genesis_block
    #block = "0000000000000000000bacba8a879d2dbb92918a64d896ad64c0dd86faa10405"
    begin
        data = JSON.parse(br.read("headers/2000/#{block}"))
        data.each do |b|
            f.puts "#{b["hash"]}\t#{b["height"]}\t#{b["time"]}\t#{b["mediantime"]}\t#{b["difficulty"]}\t#{b["nTx"]}"
            block = b["hash"]
        end
        block = data.last["nextblockhash"]
        puts data.last["height"]
    end while !block.nil? 
end
