require 'mkmf'

# Create a Makefile for building this extension
#
# export PINSDK=$CVSTOP/portal
# ruby ./extconf.rb --with-portal-lib=$PINSDK/lib --with-portal-include=$PINSDK/include

sdk = ENV["PINSDK"] || "/usr/local/portal"
dir_config("portal", sdk)
if have_header( "pcm.h" ) and
   have_library( "portal", "pin_log_flist" )
   create_makefile( "portalext" )
end
