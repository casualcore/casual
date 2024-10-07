//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/administration/cli.h"

#include "common/exception/guard.h"
#include "common/environment.h"

// TODO remove in 1.7
#include <sys/stat.h>

namespace casual
{
   using namespace common;

   namespace administration
   {
      namespace local
      {
         namespace
         {
            namespace detail
            {
               // TODO remove in 1.7
               // does pretty much exactly what std::filesystem::equivalent does, on posix
               bool equivalent( const std::filesystem::path& lhs, const std::filesystem::path& rhs)
               {
                  auto get_stat = []( auto& path) -> std::optional< struct ::stat>
                  {
                     struct ::stat stat{};
                     if( ::stat( path.c_str(), &stat) == 0)
                        return stat;

                     return {};
                  };

                  auto ls = get_stat( lhs);
                  auto rs = get_stat( rhs);

                  if( ls && rs)
                     return ls->st_dev == rs->st_dev && ls->st_ino == rs->st_ino;

                  return false;
               }
               
            } // detail

            void validate_preconditions()
            {
               // TODO replace with std::filesystem::equivalent in 1.7
               // std::filesystem::equivalent is not implemented in the g++ version we
               // build casual 1.6 with
               // 1.7 Update: std::filesystem::equivalent seems to fail randomly in some cases... (1 in 100 ish)
               //    error: [generic:Operation not supported] in equivalent: Operation not supported
               //    not sure why... Revert to our own stuff...
               if( detail::equivalent( environment::log::path(), "/dev/stdout"))
                  code::raise::error( code::casual::preconditions, "casual log can't be tied stdout when using cli");
            }


            void main( int argc, const char** argv)
            {
               validate_preconditions();
               administration::cli::parse( argc, argv);
            }

         } // <unnamed>
      } // local
      
   } // administration

} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::administration::local::main( argc, argv);
   });
}


