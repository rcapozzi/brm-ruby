brm-ruby: Ruby bindings for Oracle BRM (formally Portal's Infranet).
    by Raymond Capozzi


== DESCRIPTION:

brm-ruby enables Ruby programs to communicate with Oracle BRM (aka Portal's Infranet.


== SYNOPSIS:

	require 'brm-ruby'
	
	portal = Portal::Connection.new
	portal.loopback
	portal.xop(:PCM_OP_CUST_DELETE_ACCT,:PIN_FLD_POID => "0.0.0.1 /account 11")
	
This creates a connection to Portal using a pin.conf and deletes an account.


== REQUIREMENTS:

You need portal.[so|dll] to run.
You need a 32-bit ruby. The Oracle BRM libraries for the CM are 32bit. You cannot run
a 64-bit ruby and use this extension.


== INSTALL

You need include files to build. 
Under non-win32, the setup.rb should be enough. I'm still trying to understand
how one explains configuring VS. Perhaps I'll release win32 builds.

== hacking

ruby -I lib:ext:test -r portal -e 'Portal::Connection.new.connect(nil).xop(:PCM_OP_READ_OBJ,{:PIN_FLD_POID=>"0.0.0.1 /account 1"})'
ruby -I lib:ext:test -r portal -e 'hash=Portal::Connection.new.connect(nil).xop(:PCM_OP_READ_OBJ,{:PIN_FLD_POID=>"0.0.0.1 /account 1"}); Portal.test_flist_to_hash(hash)'
ruby -I lib:ext:test -r portal -e 'puts Portal.test_flist_to_hash(eval(File.read("hash.rb")))'

cd ext; make && cd - && ruby -I lib:ext:test -r pp -r portal -e 'Portal.set_log_level("debug"); puts Portal.hash_to_flist_string(eval(File.read("hash.rb")))'
