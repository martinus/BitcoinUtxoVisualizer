# based on https://github.com/codekitchen/ruby-protocol-buffers/blob/master/lib/protocol_buffers/runtime/varint.rb
class VarInt
	def self.encode_uint(io, int_val)
		raise "negatives numbers are not supported" if int_val < 0
		
		loop do
			byte = int_val & 0b0111_1111
			int_val >>= 7
			if int_val == 0
				io << byte.chr
				break
			else
				io << (byte | 0b1000_0000).chr
			end
		end
	end

	def self.decode_uint(io)
		int_val = 0
		shift = 0
		loop do
			raise "too many bytes when decoding varint" if shift >= 64
			byte = io.getbyte
			int_val |= (byte & 0b0111_1111) << shift
			shift += 7
			return int_val if (byte & 0b1000_0000) == 0
		end
	end

	def self.decode_int(io)
		v = decode_uint(io)
		(v >> 1) ^ -(v & 1)
	end
	
	def self.encode_int(io, int_val)
		v = (int_val << 1) ^ (int_val >> 63)
		encode_uint(io, v)
	end
end


if __FILE__ == $0
    require "pp"

	a = 0
	str = ""
	VarInt.encode_int(str, a)
	b = VarInt.decode_int(StringIO.new(str))
	pp [a, b, str.size, str]
end