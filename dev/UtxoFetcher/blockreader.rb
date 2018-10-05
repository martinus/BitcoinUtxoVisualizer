require 'net/http'
require 'uri'

# Reads JSON block data from bitcoin server via the JSON interface.
# see https://github.com/bitcoin/bitcoin/blob/master/doc/REST-interface.md
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

