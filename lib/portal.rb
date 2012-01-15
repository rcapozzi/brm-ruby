#--
# Copyright (c) 200x
#++

$:.unshift(File.dirname(__FILE__)) unless
  $:.include?(File.dirname(__FILE__)) ||
  $:.include?(File.expand_path(File.dirname(__FILE__)))

require 'portalext'
module Portal #:nodoc:

	# Fields from pcm.h. Aquire with
	# ruby -ne 'if $_ =~ /^\#define\s+(PIN_FLDT_.*)\s+(\d+)(.*)$/; puts "%-30s = %2s # %s" %[$1,$2,$3];end' pcm.h
	# Reference with ::
	PIN_FLDT_UNUSED     = 0
	PIN_FLDT_INT        = 1
	PIN_FLDT_UINT       = 2   # /* OBSOLETE */
	PIN_FLDT_ENUM       = 3
	PIN_FLDT_NUM        = 4   # /* OBSOLETE */
	PIN_FLDT_STR        = 5
	PIN_FLDT_BUF        = 6
	PIN_FLDT_POID       = 7
	PIN_FLDT_TSTAMP     = 8
	PIN_FLDT_ARRAY      = 9   # /* array element */
	PIN_FLDT_SUBSTRUCT  = 10  # /* sub-type substructure */
	PIN_FLDT_OBJ        = 11  # /* whole object */
	PIN_FLDT_BINSTR     = 12  # /* (short) binary string data */
	PIN_FLDT_ERR        = 13
	PIN_FLDT_DECIMAL    = 14
	PIN_FLDT_TIME       = 15
	PIN_FLDT_TEXTBUF    = 16
	PIN_FLDT_ERRBUF     = PIN_FLDT_ERR
	PIN_FLDT_LAST       = 16
	PIN_ERROR_LEVEL_ERROR = 1
	PIN_ERROR_LEVEL_WARN  = 2
	PIN_ERROR_LEVEL_DEBUG = 3

end

# Requires
# eg: require 'portal/constants'

# Add all the stuff to the module
Portal.class_eval do
  # include Portal::Connection
end

