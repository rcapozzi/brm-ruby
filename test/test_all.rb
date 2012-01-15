#--
# Run all the tests.
#
# Copyright (c) 200x
#++

require 'rubyunit'

Dir.chdir File.dirname( __FILE__ )
Dir["**/test_*.rb"].each { |file| load file }
