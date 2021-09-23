require 'minitest/autorun'
require 'brm-jruby'
require 'rainbow'

class BrmTest < Minitest::Test
	@xop_debug = false
	attr_accessor :ctxp
	def assert_error_info_result(resp, value)
    assert(resp.has_key?("PIN_FLD_ERROR_INFO"), "Expect PIN_FLD_ERROR_INFO key")
    assert_equal(value, resp["PIN_FLD_ERROR_INFO"][0]["PIN_FLD_RESULT"], "Expect error_info.result == #{value}")
   end

	def read_obj_by(klass, fld_name, fld_value, read_obj=0)
    resp = find_by(klass, fld_name, fld_value)
    if poid = resp["PIN_FLD_RESULTS"][read_obj]
      return read_obj(poid)
    end
  end

  def find_by(klass, fld_name, fld_value)
		req = <<~_flist_
		0 PIN_FLD_POID           POID [0] 0.0.0.1 /search -1 0
		0 PIN_FLD_FLAGS           INT [0] 512
		0 PIN_FLD_TEMPLATE        STR [0] "select X from #{klass} 1 where 1.F1 = V1 "
		0 PIN_FLD_ARGS          ARRAY [1]
		1   #{fld_name}           STR [0] "#{fld_value}"
		0 PIN_FLD_RESULTS       ARRAY [*]
		1 PIN_FLD_POID           POID [0]
		_flist_
		return xop("SEARCH", 0, FList.from_str(req), "#{klass}.#{fld_name}=#{fld_value}")
	end

	def delete_all(resp)
		ary_fld = 'PIN_FLD_RESULTS'
		poid_fld = 'PIN_FLD_POID'
		return unless resp.has_key?(ary_fld)
		resp[ary_fld].each do |k, v|
			delete_obj(v[poid_fld])
		end
	end

	def delete_obj(poid)
		return xop("DELETE_OBJ", 0, PIN_FLD_POID: poid);
	end

  def read_obj(poid)
    poid = poid["PIN_FLD_POID"] if poid.is_a?(Hash)
		return xop("READ_OBJ", 0, PIN_FLD_POID: poid);
	end

  def get_product_info(poid_or_flist)
    poid = poid_or_flist['PIN_FLD_POID'] if poid_or_flist.is_a?(Hash) 
    poid ||= poid_or_flist[:PIN_FLD_POID] if poid_or_flist.is_a?(Hash)
    poid ||= poid_or_flist
		resp = xop("PRICE_GET_PRODUCT_INFO", 0, PIN_FLD_POID: poid);
		assert_equal(1, resp["PIN_FLD_PRODUCTS"].size, 'Exact match')
		return resp["PIN_FLD_PRODUCTS"][0]
	end

	# filename: File with input flist for set pricelist
	def load_product(filename)
    doc = File.read(filename)
		flist = FList.from_str(doc)
		product = flist.to_hash
		product_code = product['PIN_FLD_PRODUCTS'][0]['PIN_FLD_CODE'] 
		#|| abort "Bad file: #{filename}"

    resp = find_by('/product', 'PIN_FLD_CODE', product_code)
    if r = resp["PIN_FLD_RESULTS"]
      r.each do |k, v|
        prod = read_obj(v["PIN_FLD_POID"])
        req = <<~_flist_.strip
        0 PIN_FLD_POID               POID [0] #{prod['PIN_FLD_POID']}
        0 PIN_FLD_PROGRAM_NAME        STR [0] "testnap"
        0 PIN_FLD_PRODUCTS          ARRAY [0]
        1     PIN_FLD_DELETED_FLAG    INT [0] 1
        1     PIN_FLD_CODE            STR [0] "#{prod['PIN_FLD_CODE']}"
        1     PIN_FLD_NAME            STR [0] "#{prod['PIN_FLD_NAME']}"
        1     PIN_FLD_TAILORMADE      INT [0] #{prod['PIN_FLD_TAILORMADE']}
        _flist_
        resp = xop('PRICE_SET_PRICE_LIST', 0, req, "Delete #{prod['PIN_FLD_POID']}")
      end
      
      # Verify nothing there
      resp = find_by('/product', 'PIN_FLD_CODE', product_code)
      assert(!resp.has_key?("PIN_FLD_RESULTS"), 'No existing product')
    else
      puts '# No existing prodcuts to delete'
    end
		
    # Create the product
    resp = xop('PRICE_SET_PRICE_LIST', 0, flist, "create product.#{product_code}")
    # Should return following for newly created product
    # {"PIN_FLD_RESULT"=>1, "PIN_FLD_PRODUCTS"=>{0=>{"PIN_FLD_PRODUCT_OBJ"=>"0.0.0.1 /product 123 0"}}, "PIN_FLD_POID"=>"0.0.0.1 /dummy 1 0"}
		assert_equal(resp['PIN_FLD_RESULT'], 1, 'PRICE_SET_PRICE_LIST should return 1');
    @product_poid = resp['PIN_FLD_PRODUCTS'][0]['PIN_FLD_PRODUCT_OBJ']
    assert(@product_poid.size > 0, 'New product created')
		product
  end

  # flist:: Can be hash, flist, or string. Always return hash
	def xop(opcode, flags, flist, label=nil)
		unless @ctxp
			@ctxp = com.portal.pcm.PortalContext.new
			@ctxp.connect
		end
		flist_hash = case flist
		when Hash; flist
		when FList;	flist.to_hash
		when String; FList.from_str(flist).to_hash
		else 'Unknown'
		end

		poid = flist_hash[:PIN_FLD_POID] || flist_hash['PIN_FLD_POID'] || 'Unknown'
		if poid =~ / -1 /
			str = flist_hash['PIN_FLD_CODE'] || flist_hash['PIN_FLD_NAME']
			poid = poid.gsub(/ -1 .*/, '.CODE=' + str) if str
		end
		puts '# xop %-35s [%s]' % [Rainbow(opcode).yellow, Rainbow(label||poid).magenta]
		if @xop_debug == true
			puts "# xop %s\n%p" % [Rainbow('input ' + flist.class.to_s).yellow, flist_hash]
		end
		File.open("pcm_session.log", 'a+') {|f| f.puts "# xop #{opcode} request"; f.puts(FList.from_hash(flist_hash)) }
		resp = @ctxp.xop(opcode, flags, flist)
		resp = case resp
		when Hash; resp
		when FList;	resp.to_hash
		when String; FList.from_str(resp).to_hash
		end
		if @xop_debug == true
			puts "# xop #{ Rainbow('output').yellow }%s (#{__FILE__}:#{__LINE__})\n%p" % [Rainbow(resp.class).yellow, resp]
		end
		File.open("pcm_session.log", 'a+') {|f| f.puts "# xop #{opcode} response"; f.puts(FList.from_hash(resp)) }
		return resp
	end

	def pre
	end

	def self.exec
		t = self.new("BRMTestX")
		`./logcheck.sh zap`
		template = "# TEST %s start: %s" % [@name, @descr]
		template = "%-70s" % template
    puts Rainbow(template).black.bg(:white) if @name
		t.pre
		t.run
    puts Rainbow("# TEST #{@name} end").black.bg(:white) if @name
	ensure
		#puts Rainbow('# Closing context').gray if @xop_debug
		t.ctxp.close(true) if t and t.ctxp
		puts `./logcheck.sh check`
	end
	def dbconnect
		# dbuser = prop.getProperty("db.user");
		# dbpass = prop.getProperty("db.pass");
		# dbhost = prop.getProperty("db.host");
		# dbport = prop.getProperty("db.port");
		# dbservicename = prop.getProperty("db.servicename");
		# try {
		# 	Class.forName("oracle.jdbc.driver.OracleDriver");
		# 	logger.info("Connecting to database...");
		# 	logger.info("jdbc:oracle:thin:" + dbuser + "/" + dbpass + "@//"
		# 			+ dbhost + ":" + dbport + "/" + dbservicename);
		# 	dbConn = DriverManager.getConnection(
		# 	"jdbc:oracle:thin:@//" + dbhost + ":" + dbport + "/"
		# 			+ dbservicename, dbuser, dbpass);
		# } catch (ClassNotFoundException e) {
		# 	e.printStackTrace();
		# 	fail("Error connecting to BRM DB");
		# } catch (SQLException e) {
		# 	e.printStackTrace();
		# 	fail("Error connecting to BRM DB");
		# }
	end

end

