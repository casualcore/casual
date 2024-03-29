//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/file.h"

namespace casual
{
   namespace common::unittest
   {
      namespace file
      {
         namespace temporary
         {
            common::file::scoped::Path content( std::string_view extension, std::string_view content);

            template< typename... C>
            std::vector< common::file::scoped::Path> contents( std::string_view extension, C&&... contents)
            {
               std::vector< common::file::scoped::Path> result;
               ( result.push_back( temporary::content( extension, contents)) , ...);
               return result;
            }

            common::file::scoped::Path name( std::string_view extension);
         } // temporary

         common::file::scoped::Path content( const std::filesystem::path& file, std::string_view content);

         platform::size::type size( const std::filesystem::path& path);
         bool empty( const std::filesystem::path& path);

         namespace fetch
         {
            //! @return the extracted content of the file.
            std::string content( const std::filesystem::path& path);

            namespace until
            {
               //! Tries to fetch the content of a file until there are content...
               //! @return the extracted content of the file.
               std::string content( const std::filesystem::path& path);
               
            } // until
            
         } // fetch

         //! @returns true if a line in the file matches the regular `expression`
         //! @note has to match the whole line.
         bool contains( const std::filesystem::path& path, std::string_view expression);

      } // file

      namespace directory
      {
         namespace temporary
         {
            //! creates an unique directory
            //! and deletes all content and the directory on destruction
            struct Scoped 
            {
               Scoped();
               ~Scoped();

               Scoped( Scoped&&) noexcept;
               Scoped& operator = ( Scoped&&) noexcept;

               Scoped( const Scoped&) = delete;
               Scoped& operator =( const Scoped&) = delete;

               inline auto& path() const & { return m_path;}
               inline auto string() const { return m_path.string();}
               inline operator const std::filesystem::path&() const & { return m_path;}

               friend std::ostream& operator << ( std::ostream& out, const Scoped& value);

            private:
               std::filesystem::path m_path;
            };
            
         } // temporary
      } // directory


   } // common::unittest
} // casual


