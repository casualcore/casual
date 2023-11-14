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
#include "common/string/compose.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"
#include "common/code/convert.h"
#include "common/result.h"

// std
#include <cstdio>
#include <iostream>

// posix
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
                  return strong::file::descriptor::id{ ::fileno( stdin)};
               }

               strong::file::descriptor::id out()
               {
                  return strong::file::descriptor::id{ ::fileno( stdout)};
               }
            } // standard
         } // descriptor

         namespace local
         {
            namespace
            {
               const std::filesystem::path& create_parent( const std::filesystem::path& path)
               {
                  directory::create( path.parent_path());
                  return path;
               }
               
            } // <unnamed>
         } // local

         Input::Input( std::filesystem::path path) 
            : m_stream{ path, std::ios::in }, m_path{ std::move( path)}
         {
            if( ! m_stream.is_open())
               code::raise::error( code::casual::invalid_path, "failed to open file: ", *this);
         }

         std::ostream& operator << ( std::ostream& out, const Input& value) 
         { 
            return stream::write( out, value.m_path);
         }

         static_assert( concepts::movable< Input>);


         Output::Output( std::filesystem::path path, std::ios::openmode mode)
            : std::ofstream{ local::create_parent( path), std::ios::out | mode}, m_path{ std::move( path)}
         {}

         //! @returns a new/reopened file with the same path
         Output Output::reopen( std::ios::openmode mode) &&
         {
            return Output{ std::move( m_path), mode};
         }
         
         std::ostream& operator << ( std::ostream& out, const Output& value)
         {
            return out << value.m_path;
         }


         namespace output
         {
            base::base( std::filesystem::path path, std::ios::openmode mode) 
               : m_stream{ local::create_parent( path), std::ios::out | mode}, m_path{ std::move( path)}
            {
               if( ! m_stream.is_open())
                  code::raise::error( code::casual::invalid_path, "failed to open file: ", *this);
            }

            void base::reopen( std::ios::openmode mode)
            {
               m_stream.close();
               m_stream.open( local::create_parent( m_path), mode);
               if( ! m_stream.is_open())
                  code::raise::error( code::casual::invalid_path, "failed to reopen file: ", *this);
            }

            std::ostream& operator << ( std::ostream& out, const base& value) 
            {
               return stream::write( out, value.m_path);
            }

            static_assert( concepts::movable< Truncate>);
            
         } // output
         

         void remove( const std::filesystem::path& path)
         {
            if( ! path.empty())
            {
               if( std::filesystem::remove( path))
                  log::line( log::debug, "removed file: ", path);
               else
                  log::line( log::category::error, code::casual::invalid_path, " failed to remove file: ", path);
            }
         }

         void rename( const std::filesystem::path& source, const std::filesystem::path& target)
         {
            std::filesystem::rename( source, target);
            log::line( log::debug, "moved file source: ", source, " -> target: ", target);
         }

         namespace scoped
         {
            Path::~Path()
            {
               if( ! empty())
                  file::remove( *this);
            }

            std::filesystem::path Path::release()
            {
               return std::exchange( *this, {});
            }
            
            std::ostream& operator << ( std::ostream& out, const Path& value)
            {
               //
               // Needed because std::ostream and std::filesystem::path uses std::quoted
               return out << value.string();
            }

         } // scoped


         std::vector< std::filesystem::path> find( std::string_view pattern)
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

         std::vector< std::filesystem::path> find( const std::vector< std::string>& patterns)
         {
            std::vector< std::filesystem::path> result;

            for( auto& pattern : patterns)
               algorithm::append_unique( find( pattern), result);

            return result;
         }

         namespace name
         {
            std::string unique( std::string_view prefix, std::string_view postfix)
            {
               return string::compose( prefix, uuid::string( uuid::make()), postfix);
            }
         } // name


         namespace permission
         {
            bool execution( const std::filesystem::path& path)
            {
               namespace fs = std::filesystem;
               return ( fs::status( path).permissions() & fs::perms::owner_exec) != fs::perms::none;
            }
         } // permission

         std::filesystem::path temporary( std::string_view extension)
         {
            return environment::directory::temporary() / string::compose( uuid::make(), '.', extension); 
         }

      } // file

      namespace directory
      {
         std::filesystem::path create( std::filesystem::path path)
         {
            if( ! std::filesystem::exists( path))
               std::filesystem::create_directories( path);

            return path;
         }

         std::filesystem::path create_parent_path( std::filesystem::path path)
         {
            create( path.parent_path());
            return path;
         }

         namespace shared
         {
            std::filesystem::path create( std::filesystem::path path)
            {
               namespace fs = std::filesystem;
               if( ! fs::exists( path))
               {
                  fs::create_directories( path);
                  fs::permissions( path, fs::perms::group_all, fs::perm_options::add);
               }

               return path;
            }
         } // shared
      } // directory

   } // common
} // casual


