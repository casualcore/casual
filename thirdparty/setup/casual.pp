case $operatingsystem 
{
  'Solaris':   
   { 
      err("Platform not supported!")
   }
   'RedHat', 'CentOS':
   { 
      package { 'uuid':
	     name => 'libuuid-devel',
	     ensure => installed
      }
      package { 'yaml':
	     name => 'libyaml-cpp0.3-dev',
	     ensure => installed
      }
      package { 'xml':
	     name => 'pugixml-devel',
	     ensure => installed
      }
      package { 'sqlite3':
	     name => 'sqlite3-devel',
	     ensure => installed
      }
      warning('Install yaml-cpp-release-0.3.0 manually with sudo python $CASUAL_BUILD_HOME/thirdparty/setup/install_yaml.py')
   }
   /^(Debian|Ubuntu)$/:
   {
      package { 'uuid':
	     name => 'uuid-dev',
	     ensure => installed
      }
      package { 'yaml':
	     name => 'libyaml-cpp0.3-dev',
	     ensure => installed
      }
      package { 'json':
	     name => 'libjson-c-dev',
	     ensure => installed
      }
      package { 'xml':
	     name => 'libpugixml-dev',
	     ensure => installed
      }
      package { 'sqlite3':
	     name => 'libsqlite3-dev',
	     ensure => installed
      }
   }
   'Darwin' :
   {
      warning("Install thirdparties manually with port or homebrew!")
      warning('Install yaml-cpp-release-0.3.0 manually with sudo python $CASUAL_BUILD_HOME/thirdparty/setup/install_yaml.py')
   }
   default: 
   {
      err("Platform not supported!")
   }
}
