# Colorizes density data with a given colormap.
# log(density) is mapped to a color, and clamped on the upper side.
class Colorize
	# converts colormap with values [0, 1( to bytes in the range [0-255].
	def initialize(colors_01_rgb, max_value)
		@colormap_rgb = []
		colors_01_rgb.each do |r, g, b|
			r = truncate(0, (256 * r).to_i, 255)
			g = truncate(0, (256 * g).to_i, 255)
			b = truncate(0, (256 * b).to_i, 255)
			
			@colormap_rgb.push [r, g, b].pack("CCC")
		end

		# calculates the factor so that max_value is the last integer value that mapps to 255.
		@fact = 256.0 / Math.log(max_value + 1)
	end

	def truncate(min, val, max)
		if val < min
			min
		elsif val > max
			max
		else
			val
		end
	end
	
	# converts density data into RGB byte values
	def colorize(density_data, out_stream)
		background = [0, 0, 0].pack("CCC") # black
		#background = [255, 255, 255].pack("CCC") # white
		
		density_data.each do |x|
			if x == 0
				out_stream << background
			else				
				x = @fact * Math.log(x)
				
				x = truncate(0, x.to_i, 255)
				out_stream << @colormap_rgb[x]
			end
		end
	end
end

