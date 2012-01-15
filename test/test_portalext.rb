#--
# Tests for the C extension
#++

require 'test/setup'
require 'test/unit'

def prompt
	puts "Any Key to continue"
	gets
end
# prompt

class TestPortal < Test::Unit::TestCase
  def setup
    # nothing to do
  end

	def test_set_program_name
		assert(Portal.set_program_name("A Test"))
	end

	def test_set_log_level
		e = Portal::Error
		assert_raises(e) do Portal.set_log_level(:foo) end
		assert_raises(e) do Portal.set_log_level(99) end
		%w(error warn debug).each do |level|
			assert(Portal.set_log_level(level.upcase))
			assert(Portal.set_log_level(level.to_sym))
		end
		assert(Portal.set_log_level(3))
	end

	def test_field_number
		assert_equal(16, Portal.field_num("PIN_FLD_POID"))
		assert_equal(16, Portal.field_num(:PIN_FLD_POID))
	end

	def test_field_type
		assert_equal(7, Portal.field_type("PIN_FLD_POID"), "String key")
		assert_equal(7, Portal.field_type(:PIN_FLD_POID), "Symbol key")
	end

	def test_field_name
		assert_equal("PIN_FLD_ERR_BUF", Portal.field_name("1"), "Name using string")
		assert_equal("PIN_FLD_ERR_BUF", Portal.field_name(1), "Name using int")
	end

	def test_portal_convert_str
		hash = {"PIN_FLD_PROGRAM_NAME" => "string01", "PIN_FLD_NAME" => "string02"}
		assert_equal(hash,Portal.test_flist_to_hash(hash))
	end

	# Keys are strings. Values are ints or floats for certain field types.
	def create_expected_results(hash)
		return hash.inject({}){|obj,memo|k,v=memo[0].to_s,memo[1].to_i;obj[k] = v;obj}
	end

	def ex_portal_numeric(hash)
		out = create_expected_results(hash)
		assert_equal(out,Portal.test_flist_to_hash(hash))
	end

	def test_portal_convert_enum
		ex_portal_numeric({:PIN_FLD_PAY_TYPE => 10003, "PIN_FLD_DIRECTION" => "0" })
	end

	def test_portal_convert_int
		ex_portal_numeric({"PIN_FLD_COUNT" => 1, :PIN_FLD_FLAGS => 256})
	end

	def test_hash_to_flist_tstamp
		ex_portal_numeric({"PIN_FLD_CREATED_T" => "1", :PIN_FLD_CREATED_T => 1167900875})
	end

	def test_hash_to_flist_decimal
		hash = {"PIN_FLD_CREDIT_LIMIT" => "1",   :PIN_FLD_CREDIT_FLOOR => 8,
		        "PIN_FLD_CURRENT_BAL" => "1.1", :PIN_FLD_RESERVED_AMOUNT => 1.1,}
		#ex_portal_numeric(hash)
		assert(Portal.test_flist_to_hash(hash))
	end

	def test_hash_to_flist_poid
		hash = {"PIN_FLD_POID" => "0.0.0.1 /account 1 2"}
		assert_equal(hash,Portal.test_flist_to_hash(hash))

		hash = {"PIN_FLD_POID" => "0.0.0.1 /account 1"}
		good = {"PIN_FLD_POID" => "0.0.0.1 /account 1 0"}
		assert_equal(good,Portal.test_flist_to_hash(hash))
	end

	def test_hash_to_flist_struct
		hash = {
			"PIN_FLD_NAME" => "Foo",
			:PIN_FLD_EVENT => {
				0 => {
					:PIN_FLD_AAC_SERIAL_NUM => "AAC_SERIAL_NUM"
				},
			},
		}
		#assert(Portal.test_hash_to_flist(hash))
	end

	# Test the conversion of Arrays
	def test_hash_to_flist_z_array
		hash = {
			"PIN_FLD_NAME" => "Foo",
			:PIN_FLD_ARGS  => {
				1 => { "PIN_FLD_NAME" => "Furry Giant" },
				2 => {
					:PIN_FLD_NAME           => "Name 1",
					:PIN_FLD_CREATED_T      => 1,
				},
				3 => {
					:PIN_FLD_NAME           => "Value 2",
					:PIN_FLD_CREATED_T      => 2,
				},
			},
		}

		#hash.pp
		#puts "*" * 60
		assert(resp = Portal.test_flist_to_hash(hash))
		#resp.pp
	end

	def xtest_connect
		p = Portal::Connection.new
		assert(p.connect(nil))
	end

end

class Hash
  def pp(level=0)
    pad = " " * level*2
    max = collect{|item|item[0].to_s.size}.max
    min = collect{|item|item[0].to_s.size}.min

    keys.sort{|a,b|a.to_s <=> b.to_s}.each do |key|
      value = self[key]
      if value.class == Hash
        puts "%2s %s%s =>\n"%[level,pad, key.to_s.rjust(max)]
        value.pp(level+1)
      else
      puts "%2s %s%s => %s\n"%[level,pad, key.to_s.rjust(max),value]
      end

    end
  end
end
