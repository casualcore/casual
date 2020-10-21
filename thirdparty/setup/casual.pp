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
      package { 'sqlite3':
	     name => 'sqlite3-devel',
	     ensure => installed
      }
   }
   /^(Debian|Ubuntu)$/:
   {
      package { 'uuid':
	     name => 'uuid-dev',
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
   }
   default: 
   {
      err("Platform not supported!")
   }
}
