# Test the conntion aspects of the project
# I run a cm_proxy on my localhost. It make the connecting a little faster.

require 'test/setup'

if __FILE__ == $0
	puts "#{__FILE__}: Start."

	ph = Portal::Connection.new
	request = {
		:PIN_FLD_POID => "0.0.0.1 /service/pcm_client 1",
		:PIN_FLD_TYPE => 0,
	}
	points =  {
			0 => { :PIN_FLD_CM_PTR => "ip localhost 11960"}
		}
	request[:PIN_FLD_CM_PTRS] = points
	puts "Connect in:\n%p\n" % [request]
	out = ph.connect(request)
	puts "Connect out:\n%p\n" % [out]

	puts "#{__FILE__}: Done."
end
