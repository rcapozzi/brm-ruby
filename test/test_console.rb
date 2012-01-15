# Console-based tests for running from Visual Studio

case RUBY_PLATFORM
	when /mswin32/
		ENV["PATH"] += ";.;./lib;C:/Program Files/Portal Software/PortalDevKit/lib"
		#$:.unshift("Debug")
		dir = File.dirname(__FILE__) + '/../Debug'
		$:.unshift(dir) if File.exist?(dir)
	else
		$:.unshift("ext")
end
$:.unshift("lib")

require 'portal'

# Quick and dirty ways to pause the run untill user input
def prompt
	puts "Any Key to continue"
	gets
end

#
#
#
if __FILE__ == $0
	puts "#{__FILE__}: Start."

	#1.times do
		#Thread.new do
			ph = Portal::Connection.new
			puts "Connecting returns %p" % ph.connect(nil)
			request = {:PIN_FLD_POID => "0.0.0.1 /event/billing/cycle/tax 237793582760533607"}

			#response = ph.robj(request,:return=>:flist_string)

			response = ph.robj("0.0.0.1 /event/billing/cycle/tax 237793582760533607")
			puts "robj\n%p" % response

			#puts "hash to flist string:\n%s\n" % Portal.hash_to_flist_string(response)
			# p.loopback
			#puts p.session
			#sleep 1
			#puts p.userid
			#puts "%p" % p.robj(:PIN_FLD_POID => "0.0.0.1 /account 1")
		#end
	#end

	#sleep 5
	puts "#{__FILE__}: Done."
end
