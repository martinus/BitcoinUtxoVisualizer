class Colorize
	def self.truncate(min, val, max)
		if val < min
			min
		elsif val > max
			max
		else
			val
		end
	end
	
	def self.colormap_conversion(colors_01_rgb)
		rgb = []
		colors_01_rgb.each do |r, g, b|
			r = truncate(0, (256 * r).to_i, 255)
			g = truncate(0, (256 * g).to_i, 255)
			b = truncate(0, (256 * b).to_i, 255)
			
			rgb.push [r, g, b].pack("CCC")
		end
		rgb
	end	
	
	def self.calc_clamp_factor(max_value)
		256.0 / Math.log(max_value + 1)
	end
	
	def self.colorize(data, colormap, clamp_max)
		cm = colormap_conversion(colormap)
	
		background = [0, 0, 0].pack("CCC") # black
		#background = [255, 255, 255].pack("CCC") # white
		
		fact = calc_clamp_factor(clamp_max)
		
		data = data.map do |x|
			if x == 0
				background
			else				
				x = fact * Math.log(x)
				
				x = truncate(0, x.to_i, 255)
				cm[x]
			end
		end
		
		data.join
	end
end

