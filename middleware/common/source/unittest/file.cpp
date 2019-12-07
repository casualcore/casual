//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest/file.h"

#include "common/log/category.h"
#include "common/code/system.h"
#include "common/exception/handle.h"

#include <fstream>

#include <ftw.h>


namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace file
         {
            namespace temporary
            {
               common::file::scoped::Path name( const std::string& extension)
               {
                  return { common::file::name::unique( common::directory::temporary() + "/mockup-", extension)};
               }

               common::file::scoped::Path content( const std::string& extension, const std::string& content)
               {
                  auto path = temporary::name( extension);
                  std::ofstream file{ path};
                  file << content;
                  return path;
               }
            } // temporary
         } // file

         namespace directory
         {
            namespace temporary
            {
               namespace local
               {
                  namespace
                  {
                     int unlink( const char* path, const struct stat *sb, int type, struct FTW* buffer)
                     {
                        return ::remove( path);
                     }

                     void clear( const std::string& path)
                     {
                        if( ::nftw( path.c_str(), &local::unlink, 64, FTW_DEPTH | FTW_PHYS))
                           log::line( log::category::error, "failed to remove path - ", common::code::last::system::error());
                     }
                  } // <unnamed>
               } // local
               Scoped::Scoped()
                  : m_path{ common::file::name::unique( common::directory::temporary() + "/mockup-") }
               {
                  common::directory::create( m_path);
               }
               Scoped::~Scoped()
               {
                  if( ! m_path.empty())
                  {
                     common::exception::guard( [&path = m_path]()
                     {
                        local::clear( path);
                     });
                  }  
               }

               Scoped::Scoped( Scoped&& rhs) noexcept
                  : m_path{ std::exchange( rhs.m_path, {})}
               {
               }

               Scoped& Scoped::operator = ( Scoped&& rhs) noexcept
               {
                  std::swap( m_path, rhs.m_path);
                  return *this;
               }

               std::ostream& operator << ( std::ostream& out, const Scoped& value)
               {
                  return out << value.path();
               }

            } // temporary
         } // directory

      } // unittest
   } // common
} // casual
