require "./varint.rb"

# Serializes change data from a single block into a compact and fast to read binary format.
class ChangeSerializer
    def initialize(filename)
        @filename = filename
    end

    def begin_block(block_height)
        @block_height = block_height
        @changes = []
    end

    def end_block        
        begin            
            File.open(@filename, "ab") do |f|
                f.write "BLK\0"
                f.write [@block_height].pack("L") # uint32_t
                
                @changes.sort!

                # int64_t amount, uint32_t block_height
                str = ""
                str << @changes.first.pack("qL")
                previous_amount = @changes.first[0]
                previous_block_height = @changes.first[1]

                (1...@changes.size).each do |i|
                    amount, block_height = @changes[i]

                    VarInt.encode_uint(str, amount - previous_amount)
                    VarInt.encode_int(str, block_height - previous_block_height)

                    previous_amount = amount
                    previous_block_height = block_height
                end
                f.write [str.size].pack("L") # uint32_t
                f.write str
            end
            begin_block(nil)
        rescue Errno::EACCES => e
            STDERR.puts "could not open file: #{e}. Retrying in 1 second"
            sleep 1
        end
    end

    def add(block_height, amount_satoshi)
        @changes << [amount_satoshi, block_height]
    end
end

if $0 == __FILE__
    require 'test/unit'
    require 'pp'

    class MyTest < Test::Unit::TestCase
        def test_random_data
            # create data for a few random blocks
            current_block_height = 321543
            block_data = []
            10.times do
                amounts = []
                rand(20).times do
                    # generate random data
                    block_height = rand(current_block_height + 1)
                    amount = rand((100 * 1e8).to_i)
                    amounts.push [block_height, amount, rand(2) == 0]
                end

                block_data.push [current_block_height, amounts]
                current_block_height += 1
            end

            # serialize it
            filename = "../../out/tmp.bin"
            FileUtils.rm_f(filename)
            cs = ChangeSerializer.new(filename)
            block_data.each do |block_height, amounts|
                cs.begin_block(block_height)
                amounts.each do |height, amount, is_added|
                    cs.add(height, amount, is_added)
                end
                cs.end_block
            end
        end
    end
end
