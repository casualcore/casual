//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/file.h"
#include "common/log.h"
#include "common/uuid.h"
#include "common/exception/system.h"
#include "common/algorithm.h"
#include "common/environment.h"
#include "common/memory.h"
#include "common/string.h"

// std
#include <cstdio>
#include <fstream>

// posix
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace casual
{
   namespace common
   {
      namespace file
      {
   
         Input::Input( std::string path) : std::ifstream( path), m_path( std::move( path)) 
         {
            if( ! is_open())
            {
               throw exception::system::invalid::File{ string::compose( "failed to open file: ", m_path)};
            }
         }

         std::string Input::extension() const { return file::name::extension( m_path);}

         Output::Output( std::string path) : std::ofstream( path), m_path( std::move( path)) 
         {
            if( ! is_open())
            {
               throw exception::system::invalid::File{ string::compose( "failed to open file: ", m_path)};
            }
         }

         std::string Output::extension() const { return file::name::extension( m_path);}


         void remove( const std::string& path)
         {
            if( ! path.empty())
            {
               if( std::remove( path.c_str()))
               {
                  log::line( log::category::error, "failed to remove file: ", path);
               }
               else
               {
                  log::line( log::debug, "removed file: ", path);
               }
            }
         }

         void move( const std::string& source, const std::string& destination)
         {
            if( ::rename( source.c_str(), destination.c_str()) == -1)
            {
               exception::system::throw_from_errno( string::compose( "source: ", source, " destination: ", destination));
            }
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



         std::string find( const std::string& path, const std::regex& search)
         {
            std::string result;

            auto directory = memory::guard( opendir( path.c_str()), &closedir);

            if( directory)
            {
               struct dirent* element = nullptr;
               while( ( element = readdir( directory.get())) != nullptr)
               {

                  if( std::regex_match( element->d_name, search))
                  {
                     if( path.back() != '/')
                     {
                        result = path + "/";
                     }

                     result += element->d_name;
                     break;
                  }
               }
            }
            return result;
         }

         std::string absolute( const std::string& path)
         {
            auto absolute = memory::guard( realpath( path.c_str(), nullptr), &free);

            if( absolute)
            {
               return absolute.get();
            }

            throw exception::system::invalid::File{ path};
         }

         namespace name
         {

            bool absolute( const std::string& path)
            {
               if( ! path.empty())
               {
                  return path[ 0] == '/';
               }
               return false;
            }

            std::string unique( const std::string& prefix, const std::string& postfix)
            {
               return prefix + uuid::string( uuid::make()) + postfix;
            }

            std::string base( const std::string& path)
            {
               auto basenameStart = std::find( path.crbegin(), path.crend(), '/');
               return std::string( basenameStart.base(), path.end());
            }


            std::string extension( const std::string& file)
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
               std::string extension( const std::string& path)
               {
                  auto extensionBegin = std::find( path.crbegin(), path.crend(), '.');
                  return std::string( path.begin(), extensionBegin.base());
               }

            } // without

            std::string link( const std::string& path)
            {
               std::vector< char> link_name( PATH_MAX);

               if( ::readlink( path.c_str(), link_name.data(), link_name.size()) == -1)
               {
                  exception::system::throw_from_errno( "file::name::link");
               }

               if( link_name.data())
               {
                  return link_name.data();
               }
               return {};
            }

         } // name


         bool exists( const std::string& path)
         {
            return access( path.c_str(), F_OK) == 0;
         }

         namespace permission
         {
            bool execution( const std::string& path)
            {
               // Check if path contains any directory, if so, we can check it directly
               if( algorithm::find( path, '/'))
               {
                  return access( path.c_str(), R_OK | X_OK) == 0;
               }
               else
               {
                  // We need to go through PATH environment variable...
                  for( auto total_path : string::split( environment::variable::get( "PATH", ""), ':'))
                  {
                     total_path += '/' + path;
                     if( access( total_path.c_str(), F_OK) == 0)
                     {
                        return access( total_path.c_str(), X_OK) == 0;
                     }
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
            {
               exception::system::throw_from_errno( "file::name::link");
            }
            return buffer;
         }


         std::string change( const std::string& path)
         {
            auto current = directory::current();

            if( chdir( path.c_str()) == -1)
            {
               exception::system::throw_from_errno( "file::name::link");
            }

            return current;
         }

         namespace scope
         {
            Change::Change( const std::string& path) : m_previous{ change( path)} {}
            Change::~Change()
            {
               change( m_previous);
            }
         } // scope


         namespace name
         {
            std::string base( const std::string& path)
            {
               // Remove trailing '/'
               auto end = std::find_if( path.crbegin(), path.crend(), []( const char value) { return value != '/';});

               end = std::find( end, path.crend(), '/');

               // To be conformant to dirname, we have to return at least '/'
               if( end == path.crend())
               {
                  return "/";
               }

               return std::string{ path.cbegin(), end.base()};
            }

         } // name

         bool exists( const std::string& path)
         {
            return memory::guard( opendir( path.c_str()), &closedir).get() != nullptr;
         }

         bool create( const std::string& path)
         {
            auto parent = name::base( path);

            if( parent.size() < path.size() && parent != "/")
            {
               // We got a parent, make sure we create it first
               create( parent);
            }

            if( mkdir( path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST)
            {
               log::line( log::category::error, "failed to create ", path, " - ", code::last::system::error());
               return false;
            }

            return true;
         }

         bool remove( const std::string& path)
         {
            if( rmdir( path.c_str()) != 0)
            {
               log::line( log::category::error, "failed to remove ", path, " - ", code::last::system::error());
               return false;
            }
            return true;
         }
      }

   } // common
} // casual


