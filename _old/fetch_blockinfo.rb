require './blockreader.rb'
require 'json'
require "pp"

genesis_hash = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"

br = BlockReader.new("http://127.0.0.1:8332")

filename = "headers.bin"

headers = []
if File.file?(filename)
    File.open(filename, "rb") do |f|
        headers = Marshal.load(f)
    end
end

nextblockhash = genesis_hash
if headers.size > 1
    nextblockhash = headers[-2]["nextblockhash"] || genesis_hash
end

loop do
    data = br.headers(nextblockhash, 2000)
    JSON.parse(data).each do |blockinfo|
        headers[blockinfo["height"]] = blockinfo
    end
    puts headers.size
    nextblockhash = headers.last["nextblockhash"]
    break if nextblockhash.nil?
end

puts "dumping..."
File.open(filename, "wb") do |f|
    Marshal.dump(headers, f)
end
puts "dumping done!"
