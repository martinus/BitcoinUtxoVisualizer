class DateToBlock
    def self.load(tsv_file) 
        bin_file = tsv_file + ".bin"

        # only load .bin if mtimes match
        if (File.exist?(bin_file) && (File.mtime(tsv_file) == File.mtime(bin_file)))
			File.open(bin_file, "rb") do |f|
				return Marshal.load(f)
            end
        else
            return DateToBlock.new(tsv_file)
        end
    end

    def initialize(tsv_file)
        @block_timestamp = []

        File.open(tsv_file, "rt") do |f|
            f.each_line do |l|
                l = l.split
                height = l[1].to_i
                mediantime = l[3].to_i
                @block_timestamp[height] = mediantime
            end
        end

        # make sure we don't have an empty spot
        @block_timestamp.each do |x|
            raise "we got a nil spot!" if x.nil?
        end

        # cache into a .bin file
        bin_file = tsv_file + ".bin"
		File.open(bin_file, "wb") do |f|
			Marshal.dump(self, f)
        end

        # make the .bin file's mtime the same as tsv_file
        mtime = File.mtime(tsv_file)
        File.utime(mtime, mtime, bin_file)
    end

    def date_to_block(date)
        height = @block_timestamp.bsearch_index { |x| x >= date }
        height = @block_timestamp.size if height.nil?
        timestamp_before = @block_timestamp[height - 1] if height > 0
        timestamp_after = @block_timestamp[height] if height < @block_timestamp.size
        [height, timestamp_before, timestamp_after]
    end
end