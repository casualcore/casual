//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/strong/id.h"

#include <string>
#include <fstream>
#include <filesystem>

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
               //! @returns file descriptor for stdin
               strong::file::descriptor::id in();
               //! @returns file descriptor for stdout
               strong::file::descriptor::id out();
               
            } // standard
         } // descriptor

         struct Input
         {
            Input( std::filesystem::path path);

            inline const auto& path() const noexcept { return m_path;}
            inline operator std::istream& () noexcept { return m_stream;}

            friend std::ostream& operator << ( std::ostream& out, const Input& value);

         private:
            std::ifstream m_stream;
            std::filesystem::path m_path;
         };

         struct Output : std::ofstream
         {
            //! does not throw
            Output( std::filesystem::path path, std::ios::openmode mode = std::ios::app);

            //! @returns a new/reopened file with the same path and mode.
            Output reopen( std::ios::openmode mode = std::ios::app) &&;

            inline const auto& path() const noexcept { return m_path;}
            
            friend std::ostream& operator << ( std::ostream& out, const Output& value);

         private:
            std::filesystem::path m_path;
         };

         namespace output
         {
            struct base
            {
               base( std::filesystem::path path, std::ios::openmode mode);

               void reopen( std::ios::openmode mode);
               inline decltype( auto) flush() { return m_stream.flush();}
               inline auto& path() const noexcept { return m_path;}

               template< typename T>
               auto& operator << ( T&& value) { return m_stream << std::forward< T>( value);}
               
               inline operator std::ostream& () noexcept { return m_stream;}
               
               friend std::ostream& operator << ( std::ostream& out, const base& value);

            private:
               std::ofstream m_stream;
               std::filesystem::path m_path;
            };

            template< std::ios::openmode mode>
            struct basic : base
            {
               explicit basic( std::filesystem::path path) 
                  : base{ std::move( path), mode} {}

               void reopen() { base::reopen( mode);}
            };

            using Truncate = basic< std::ios::trunc>;
            
         } // output

         void remove( const std::filesystem::path& path);

         //! Moves/renames a file from @p source to @p target
         void rename( const std::filesystem::path& source, const std::filesystem::path& target);

         namespace scoped
         {
            struct Path : std::filesystem::path
            {  
               using std::filesystem::path::path;

               Path( const Path&) = delete;
               Path& operator = ( const Path&) = delete;

               Path( Path&&) = default;
               Path& operator = ( Path&&) = default;

               ~Path();

               std::filesystem::path release();

               friend std::ostream& operator << ( std::ostream& out, const Path& value);
            };

         } // scoped
         
         //! @return found paths that matches _glob_ pattern(s)
         //! explained at:
         //!   * https://man7.org/linux/man-pages/man7/glob.7.html
         //!   * https://en.wikipedia.org/wiki/Glob_(programming)
         //! @{
         std::vector< std::filesystem::path> find( std::string_view pattern);
         std::vector< std::filesystem::path> find( const std::vector< std::string>& patterns);
         //! @}

         namespace name
         {
            //! @return a unique file-name, with post- and pre-fix, if provided
            std::string unique( std::string_view prefix = "", std::string_view postfix = "");
         } // name

         namespace permission
         {
            //! @return true if calling process has execution permission on @p path
            //! @note returns false if the file does not exists
            bool execution( const std::filesystem::path& path);

         } // permission

         //! @returns a temporary path with the provided extension
         std::filesystem::path temporary( std::string_view extension);
      } // file

      namespace directory
      {
         //! creates all directories recursively if missing.
         //! takes soft links into account when creating the path, if any.
         std::filesystem::path create( std::filesystem::path path);

         //! creates all parent directories, but not the leaf
         std::filesystem::path create_parent_path( std::filesystem::path path);

         namespace shared
         {
            //! creates directory with group rwx.
            std::filesystem::path create( std::filesystem::path path);
         } // shared

      } // directory

   } // common
} // casual


