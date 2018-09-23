# Serializes change data from a single block into a compact and fast to read binary format.
class ChangeSerializer
    def initialize(filename)
        @filename = filename
    end

    def begin_block(block_height)
        @block_height = block_height
        @amount_changes_packed = ""
        @num_amounts = 0
    end

    def end_block
        File.open(@filename, "ab") do |f|
            # write header
            # 32bit unsigned block height
            # 32bit unsigned number of amounts
            f.write [@block_height, @num_amounts].pack("LL")       
            f.write @amount_changes_packed
        end
        begin_block(nil)
    end

    def add(block_height, amount_satoshi, is_added)
        # 32bit unsigned block height
        # 64bit signed amount
        # see https://www.rubydoc.info/stdlib/core/Array%3Apack
        @amount_changes_packed << [block_height, is_added ? amount_satoshi : -amount_satoshi].pack("Lq")
        @num_amounts += 1
    end

    def self.read(filename)
        block_data = []
        File.open(filename, "rb") do |f|
            while !f.eof?
                current_block_height, num_amounts = f.read(4+4).unpack("LL")
                amounts = []
                num_amounts.times do
                    block_height, amount_satoshi = f.read(4 + 8).unpack("Lq")
                    is_added = true
                    if (amount_satoshi < 0)
                        is_added = false
                        amount_satoshi = -amount_satoshi
                    end
                    amounts.push [block_height, amount_satoshi, is_added]
                end
                block_data.push [current_block_height, amounts]
            end
        end
        block_data
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

            # load data
            t = Time.new
            read_data = ChangeSerializer::read(filename)
            #pp read_data
            assert_equal(block_data, read_data)
        end
    end
end
