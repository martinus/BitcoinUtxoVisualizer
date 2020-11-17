require './density.rb'

btc_max = 100000
satoshi_per_btc = 100_000_000


def each_amount(satosi_min, satoshi_max, &proc)
	satoshi = 1
	loop do
		proc.call(satoshi, true)
		return if satoshi >= satoshi_max		
		(1..9).each do |n|
			puts n
			proc.call(satoshi*n, false)
		end
		satoshi *= 10
	end
end

density = Density.new(3840, 2160, 0.00000001, 100000, 0, 550000)

width = 10
height = density.height
data = Array.new(width * height, 0)

each_amount(1, btc_max * satoshi_per_btc) do |sat, is_start|
	amount = sat.to_f / satoshi_per_btc
	
	x, y = density.block_amount_to_pixel(0, amount)

	if is_start
		puts y
	end

	
	
	n_pixel = is_start ? 10 : 3
	n_pixel.times do |x_off|
		data[y * 10 + x_off] = 0xff
	end	
end

File.open("legend.pgm", "wb") do |fout|
	fout.printf("P5\n%d %d\n255\n", width, height)
	fout.write(data.pack("C*"))
end


