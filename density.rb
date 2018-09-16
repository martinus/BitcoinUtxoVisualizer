class Density
	attr_reader :num_points, :width, :height
	
	def self.load(filename)
		if File.file?(filename)
			File.open(filename, "rb") do |f|
				return Marshal.load(f)
			end
		end
		nil
	end

	def initialize(width, height, min_btc, max_btc, min_blockid, max_blockid)
		# tries to load and if it fails, creates new
		@width = width
		@height = height
		@data = Array.new(@width*@height, 0)
	
		@k_btc, @d_btc = calc_k_d(Math.log(min_btc), 0,	Math.log(max_btc),	@height)
		@k_block, @d_block = calc_k_d(min_blockid, 0, max_blockid, @width)
	
		@num_points = 0
	end
	
	def clear
		@data.fill(0)
		@num_points = 0
	end
	
	def save(filename)
		File.open(filename, "wb") do |f|
			Marshal.dump(self, f)
		end
	end
	
	def calc_k_d(x1, y1, x2, y2)
		k = (y2.to_f - y1) / (x2 - x1)
		d = y1 - k*x1
		[k, d]
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
	
	def colormap_conversion(colors_01_rgb)
		rgb = []
		colors_01_rgb.each do |r, g, b|
			r = truncate(0, (256 * r).to_i, 255)
			g = truncate(0, (256 * g).to_i, 255)
			b = truncate(0, (256 * b).to_i, 255)
			
			rgb.push [r, g, b].pack("CCC")
		end
		rgb
	end	
	
	def block_amount_to_pixel(block_height, btc)
		pixel_x = @k_block * block_height + @d_block
		pixel_y = @height - (@k_btc * Math.log(btc) + @d_btc)
		
		#p [block_height, btc, pixel_x, pixel_y]
		[truncate(0, pixel_x.to_i, @width-1), truncate(0, pixel_y.to_i, @height-1)]
	end
	
	def add(block_height, btc)
		x, y = block_amount_to_pixel(block_height, btc)
		idx = y * @width + x
		@data[idx] += 1
		@num_points += 1
	end
	
	def calc_clamp_factor(max_value)
		256.0 / Math.log(max_value + 1)
	end
	
	def colorize(colormap, clamp_max)
		cm = colormap_conversion(colormap)
	
		background = [0, 0, 0].pack("CCC") # black
		#background = [255, 255, 255].pack("CCC") # white
		
		fact = calc_clamp_factor(clamp_max)
		
		num_saturated = 0
		
		x_max = 0
		data = @data.map do |x|
			if x == 0
				background
			else				
				x_max = [x_max, x].max
				
				x = fact * Math.log(x)
				num_saturated += 1 if x >= 256
				
				x = truncate(0, x.to_i, 255)
				cm[x]
			end
		end
		
		data.join
	end
end

