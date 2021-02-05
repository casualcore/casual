//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/file.h"
#include "common/log.h"
#include "common/uuid.h"
#include "common/algorithm.h"
#include "common/environment.h"
#include "common/memory.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"
#include "common/code/convert.h"
#include "common/result.h"

// std
#include <cstdio>
#include <fstream>

// posix
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>

namespace casual
{
   namespace common
   {
      namespace file
      {
         namespace descriptor
         {
            namespace standard
            {
               strong::file::descriptor::id in()
               {
                  return strong::file::descriptor::id{ ::fileno( ::stdin)};
               }

               strong::file::descriptor::id out()
               {
                  return strong::file::descriptor::id{ ::fileno( ::stdout)};
               }
               
            } // standard
         } // descriptor

   
         Input::Input( std::string path) : std::ifstream( path), m_path( std::move( path)) 
         {
            if( ! is_open())
               code::raise::log( code::casual::invalid_path, "failed to open file: ", m_path);
         }

         std::string Input::extension() const { return file::name::extension( m_path);}

         Output::Output( std::string path) : std::ofstream( path), m_path( std::move( path)) 
         {
            if( ! is_open())
               code::raise::log( code::casual::invalid_path, "failed to open file: ", m_path);
         }

         std::string Output::extension() const { return file::name::extension( m_path);}


         void remove( std::string_view path)
         {
            if( ! path.empty())
            {
               if( std::remove( path.data()))
                  log::line( log::category::error, code::casual::invalid_path, " failed to remove file: ", path);
               else
                  log::line( log::debug, "removed file: ", path);
            }
         }

         void move( std::string_view source, std::string_view destination)
         {
            posix::result( ::rename( source.data(), destination.data()), "source: ", source, " destination: ", destination);
            log::line( log::debug, "moved file source: ", source, " -> destination: ", destination);
         }


         namespace scoped
         {
            Path::Path( std::string path)
               : m_path( std::move( path))
            {
            }

            Path::Path() = default;


            Path::~Path()
            {
               if( ! m_path.empty())
               {
                  remove( m_path);
               }
            }

            Path::Path( Path&& rhs) noexcept : m_path( std::move( rhs.m_path))
            {
            }


            Path& Path::operator = ( Path&& rhs) noexcept
            {
               m_path = std::exchange( rhs.m_path, {});
               return *this;
            }

            Path::operator const std::string&() const &
            {
               return path();
            }

            Path::operator std::string_view() const &
            {
               return path();
            }


            const std::string& Path::path() const &
            {
               return m_path;
            }

            std::string Path::release()
            {
               return std::exchange( m_path, {});
            }

            std::ostream& operator << ( std::ostream& out, const Path& value)
            {
               return out << value.path();
            }

         } // scoped


         std::vector< std::string> find( std::string_view pattern)
         {
            Trace trace{ "common::file::find"};
            log::line( verbose::log, "pattern: ", pattern);

            auto error_callback = []( const char* path, int error) -> int
            {
               log::line( log::category::warning, static_cast< std::errc>( error), " error detected during find file - path: ", path);
               return 0;
            };

            ::glob_t buffer{};
            auto guard = memory::guard( &buffer, &::globfree);

            ::glob( pattern.data(), 0, error_callback, &buffer);

            return { buffer.gl_pathv, buffer.gl_pathv + buffer.gl_pathc};
         }

         std::vector< std::string> find( const std::vector< std::string>& patterns)
         {
            std::vector< std::string> result;

            for( auto& pattern : patterns)
               algorithm::append_unique( find( pattern), result);

            return result;
         }


         std::string absolute( std::string_view path)
         {
            auto absolute = memory::guard( realpath( path.data(), nullptr), &free);

            if( absolute)
               return absolute.get();

            code::raise::log( code::casual::invalid_path, path);
         }

         namespace name
         {

            bool absolute( std::string_view path)
            {
               if( ! path.empty())
                  return path[ 0] == '/';

               return false;
            }

            std::string unique( std::string_view prefix, std::string_view postfix)
            {
               return string::compose( prefix, uuid::string( uuid::make()), postfix);
            }

            std::string base( std::string_view path)
            {
               auto basenameStart = std::find( path.crbegin(), path.crend(), '/');
               return std::string( basenameStart.base(), path.end());
            }


            std::string extension( std::string_view file)
            {
               std::string filename = base( file);
               auto extensionEnd = std::find( filename.crbegin(), filename.crend(), '.');

               if( extensionEnd == filename.crend())
               {
                  return std::string{};
               }

               return std::string( extensionEnd.base(), filename.cend());
            }

            namespace without
            {
               std::string extension( std::string_view path)
               {
                  auto found = std::find( std::crbegin( path), std::crend( path), '.');
                  return std::string( path.begin(), found.base());
               }

            } // without

            std::string link( std::string_view path)
            {
               std::vector< char> link_name( PATH_MAX);

               posix::result( ::readlink( path.data(), link_name.data(), link_name.size()), "file::name::link");

               if( link_name.data())
                  return link_name.data();

               return {};
            }

         } // name


         bool exists( std::string_view path)
         {
            return access( path.data(), F_OK) == 0;
         }

         namespace permission
         {
            bool execution( std::string_view path)
            {
               // Check if path contains any directory, if so, we can check it directly
               if( algorithm::find( path, '/'))
                  return access( path.data(), R_OK | X_OK) == 0;
               else
               {
                  // We need to go through PATH environment variable...
                  for( auto total_path : string::split( environment::variable::get( "PATH", ""), ':'))
                  {
                     total_path.push_back( '/');
                     total_path += path;
                     if( access( total_path.data(), F_OK) == 0)
                        return access( total_path.data(), X_OK) == 0;
                  }
               }
               return false;
            }

         } // permission

      } // file

      namespace directory
      {

         std::string temporary()
         {
            return "/tmp";
         }

         std::string current()
         {
            char buffer[ platform::size::max::path];

            if( getcwd( buffer, sizeof( buffer)) == nullptr)
               code::system::raise( "directory::current");

            return buffer;
         }


         std::string change( std::string_view path)
         {
            auto current = directory::current();

            posix::result( chdir( path.data()), "directory::change");

            return current;
         }

         namespace scope
         {
            Change::Change( std::string_view path) : m_previous{ change( path)} {}
            Change::~Change()
            {
               change( m_previous);
            }
         } // scope


         namespace name
         {
            std::string base( std::string_view path)
            {
               // Remove trailing '/'
               auto end = std::find_if( std::crbegin( path), std::crend( path), []( const char value) { return value != '/';});

               end = std::find( end, std::crend( path), '/');

               // To be conformant to dirname, we have to return at least '/'
               if( end == std::crend( path))
                  return "/";

               return std::string{ std::begin( path), end.base()};
            }

         } // name

         bool exists( std::string_view path)
         {
            return memory::guard( opendir( path.data()), &closedir).get() != nullptr;
         }

         bool create( std::string_view path)
         {
            auto parent = name::base( path);

            if( parent.size() < path.size() && parent != "/")
            {
               // We got a parent, make sure we create it first
               create( parent);
            }

            if( mkdir( path.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST)
               return false;

            return true;
         }

         bool remove( std::string_view path)
         {
            if( rmdir( path.data()) != 0)
               return false;

            return true;
         }
      }

   } // common
} // casual


