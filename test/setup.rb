# Setup the environment for tests
case RUBY_PLATFORM
	when /mswin32/
		sdk = ENV["PINSDK"] || "C:/Program Files/Portal Software/PortalDevKit"
		ENV["PATH"] += ";.;./lib;#{sdk}/lib"
		#$:.unshift("Debug")
		dir = File.dirname(__FILE__) + '/../Debug'
		$:.unshift(dir) if File.exist?(dir)
	else
		$:.unshift("ext")
end
$:.unshift("lib")

require 'portal'
