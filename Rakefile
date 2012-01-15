require 'rake'
require 'rake/testtask'
require 'rake/clean'
require 'rake/gempackagetask'
require 'rake/rdoctask'
require 'fileutils'
require 'rubyforge'

include FileUtils

PKG_NAME    = "portal-ruby"
SVN_HOME    = "file:///c:/svn/local/portal-ruby"
RELEASE_DIR = "release"

def get_version
  ENV['version'] or "0.0.1"
 # ENV['version'] or abort "Must supply VERSION=x.y.z"
end

rdoc_template = ENV['template'] || "allison.rb"
#rdoc_template = "jamis.rb"

# Define the files to get doc-ed
Rake::RDocTask.new do |rdoc|
	rdoc.template = rdoc_template
  rdoc.rdoc_dir = "doc/rdoc"
  rdoc.options << '--line-numbers' << "--tab-width=2" << "--quiet"
  rdoc.options << "--inline-source" if rdoc_template =~ /allison/
  rdoc.rdoc_files.add(['README', 'lib/**/*.rb', 'doc/**/*.rdoc', 'ext/portalext.c'])
end

task :package => [:clean,:compile,:test,:rerdoc]

desc "Create the static web pages"
task :webgen do
  sh %{pushd doc/site; webgen; popd }
end

# Remote Site Section

desc "Push static files"
#task :site_webgen => [:webgen] do
task :site_webgen do
  sh %{rsync -e 'ssh -1' -azv doc/site/output/* rubyforge.org:/var/www/gforge-projects/portal-ruby/}
end

task :site_rdoc do
  sh %{ rsync -e 'ssh -1' -azv doc/rdoc/* rubyforge.org:/var/www/gforge-projects/portal-ruby/rdoc/ }
end

gem_spec = Gem::Specification.new do |spec|
  test_files = Dir.glob("test/**/*.rb") - Dir.glob("test/**/*local*")

  spec.name = PKG_NAME
  spec.version = get_version
	spec.summary = "Binding for Portal's Infranet and Oracle Telco Billing"
  spec.description = spec.summary
	spec.platform = Gem::Platform::RUBY
	spec.has_rdoc = true
	spec.extra_rdoc_files = [ "README" ]
  spec.test_files = test_files
  
  all_files = %w(README Rakefile) +
  #      Dir.glob("doc/rdoc/**/*") + 
      test_files +
      Dir.glob("ext/**/*.{h,c,rb}") +
      Dir.glob("{lib}/**/*.rb")
  spec.files = all_files.to_a.delete_if {|f| f.include?('.svn')}

  spec.require_path = "lib"
  spec.extensions = FileList["ext/**/extconf.rb"].to_a

  spec.required_ruby_version = '>= 1.8.5'

  if RUBY_PLATFORM =~ /mswin/
    # spec.files += ['Release/portalext.so']
    spec.extensions.clear
    spec.platform = Gem::Platform::WIN32
  end
  
  spec.author = "Raymond Capozzi"
  spec.email = "rcapozzi+ruby@gmail.com"
  spec.homepage = "portal-ruby.rubyforge.com"
  spec.rubyforge_project = "portal-ruby"
end

Rake::GemPackageTask.new(gem_spec) do |pkg|
  pkg.gem_spec = gem_spec
  #pkg.need_tar = true if RUBY_PLATFORM !~ /mswin/  
end

desc "Release from source control"
task :export do
	rm_rf RELEASE_DIR
	sh %{ svn export #{SVN_HOME} #{RELEASE_DIR} }
	puts "Now cd #{RELEASE_DIR} && rake gem version=x.y.z"
end

desc "Release this turd to rubyforge"
task :release => [:gem]  do |t|
	rf = RubyForge.new
	puts "Logging in"
	rf.login

	c = rf.userconfig
	c["release_notes"] = description if description
	c["release_changes"] = changes if changes
	c["preformatted"] = true

	files = ["#{pkg}.gem"].compact

	puts "Releasing #{name} v. #{version}"
	rf.add_release rubyforge_name, name, version, *files

end
