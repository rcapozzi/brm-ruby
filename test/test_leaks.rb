# Console based tests

require 'test/setup'

PIN_FLDT_STR = 5
PIN_FLDT_POID = 7

def test_one_pass
	Portal.field_name(1)
end

def test_batch
	100_000.times{|i| test_one_pass }
	100_000.times{|i| test_one_pass }
	100_000.times{|i| test_one_pass }
	100_000.times{|i| test_one_pass }
end

puts "#{__FILE__}: Start."

field = :PIN_FLD_PROGRAM_NAME

puts "Any key to continue. . ."
gets

test_batch
GC.start
puts "System warmed up. Any key to continue. . ."
gets

test_batch
GC.start
puts "Note size. Any key to continue. . ."
gets


puts "#{__FILE__}: Done."
