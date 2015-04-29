//!
//! casual_utility_file.cpp
//!
//! Created on: May 5, 2012
//!     Author: Lazan, Jon-Erik
//!

#include "common/file.h"
#include "common/log.h"
#include "common/error.h"
#include "common/uuid.h"
#include "common/exception.h"

#include <cstdio>

#include <fstream>


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

         void remove( const std::string& path)
         {
            if( !path.empty())
            {
               std::remove( path.c_str());
            }
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

            Path::Path( Path&&) noexcept = default;


            Path& Path::operator = ( Path&& rhs) noexcept
            {
               m_path = std::move( rhs.m_path);
               return *this;
            }

            Path::operator const std::string&() const
            {
               return path();
            }


            const std::string& Path::path() const
            {
               return m_path;
            }

            std::string Path::release()
            {
               return std::move( m_path);
            }

            std::ostream& operator << ( std::ostream& out, const Path& value)
            {
               return out << value.path();
            }

         } // scoped



         std::string find( const std::string& path, const std::regex& search)
         {
            std::string result;

            DIR* directory = opendir( path.c_str());

            if( directory)
            {
               struct dirent* element = nullptr;
               while( ( element = readdir( directory)) != nullptr)
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

               closedir( directory);
            }

            return result;
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

         } // name


         bool exists( const std::string& path)
         {
            std::ifstream file( path);
            return file.good();
         }

      } // file

      namespace directory
      {

         std::string current()
         {
            char buffer[ platform::size::max::path];
            if( getcwd( buffer, sizeof( buffer)) == nullptr)
            {
               throw exception::NotReallySureWhatToNameThisException( "could not get working directory");
            }
            return buffer;
         }


         std::string change( const std::string& path)
         {
            auto current = directory::current();

            if( chdir( path.c_str()) == -1)
            {
               throw exception::invalid::Argument{ "failed to change working directory", CASUAL_NIP( error::string())};
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

               //
               // Remove trailing '/'
               //
               auto end = std::find_if( path.crbegin(), path.crend(), []( const char value) { return value != '/';});


               end = std::find( end, path.crend(), '/');

               //
               // To be conformant to dirname, we have to return at least '/'
               //
               if( end == path.crend())
               {
                  return "/";
               }

               return std::string{ path.cbegin(), end.base()};
            }

         } // name


         bool create( const std::string& path)
         {
            auto parent = name::base( path);

            if( parent.size() < path.size() && parent != "/")
            {

               //
               // We got a parent, make sure we create it first
               //
               create( parent);
            }

            if( mkdir( path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST)
            {
               log::error << "failed to create " << path << " - " << error::string() << std::endl;
               return false;
            }

            return true;
         }

         bool remove( const std::string& path)
         {
            if( rmdir( path.c_str()) != 0)
            {
               log::error << "failed to remove " << path << " - " << error::string() << std::endl;
               return false;
            }
            return true;
         }
      }

   } // common
} // casual


