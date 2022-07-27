//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest/file.h"

#include "common/log/category.h"
#include "common/code/system.h"
#include "common/exception/guard.h"
#include "common/process.h"

#include <fstream>

namespace casual
{
   namespace common::unittest
   {
      namespace local
      {
         namespace
         {
            auto prefix()
            {
               return common::directory::temporary() / "unittest-";
            }
         } //
      } // local

      namespace file
      {
         namespace temporary
         {
            common::file::scoped::Path name( std::string_view extension)
            {
               return { common::file::name::unique( local::prefix().string(), extension)};
            }

            common::file::scoped::Path content( std::string_view extension, std::string_view content)
            {
               auto path = temporary::name( extension);
               std::ofstream file{ path};
               file << content;
               return path;
            }

         } // temporary

         common::file::scoped::Path content( const std::filesystem::path& path, std::string_view content)
         {
            std::ofstream file{ path};
            file << content;
            return { path};
         }

         platform::size::type size( const std::filesystem::path& path)
         {
            return std::filesystem::file_size( path);
         }

         namespace fetch
         {
            std::string content( const std::filesystem::path& path)
            {
               std::ifstream file{ path};
               std::stringstream stream;
               stream << file.rdbuf();
               return std::move( stream).str();
            }

            namespace until
            {
               std::string content( const std::filesystem::path& path)
               {
                  auto count = 400;
                  while( count-- > 0)
                  {
                     if( ! file::empty( path))
                        return fetch::content( path);

                     process::sleep( std::chrono::milliseconds{ 4});
                  }
                  return {};
               }
               
            } // until
            
         } // fetch

      } // file

      namespace directory
      {
         namespace temporary
         {

            Scoped::Scoped()
               : m_path{ common::file::name::unique( local::prefix().string()) }
            {
               common::directory::create( m_path);
            }

            Scoped::~Scoped()
            {
               std::filesystem::remove_all( m_path);
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

   } // common::unittest
} // casual
