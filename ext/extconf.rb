require 'mkmf'

# Create a Makefile for building this extension
#
# export PINSDK=$CVSTOP/portal
# ruby ./extconf.rb --with-portal-lib=$PINSDK/lib --with-portal-include=$PINSDK/include
append_cflags('-D_DEBUG=1') if ENV["DEBUG"]=="1"
sdk = ENV["PINSDK"] || ENV['PIN_HOME'] || "/usr/local/portal"
dir_config("portal", sdk + '/include', sdk + '/lib64')
if have_header( "pcm.h" ) and
   have_library( "portal", "pini_flist_get" )
   create_makefile( 'portalext/portalext' )
end
