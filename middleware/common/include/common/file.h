//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/move.h"
#include "common/strong/id.h"

#include <string>
#include <regex>
#include <fstream>

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

         class Input : public std::ifstream
         {
         public:
            Input( std::string path);

            const std::string& path() const  { return m_path;} 
            std::string extension() const;

            inline friend std::ostream& operator << ( std::ostream& out, const Input& value) { return out << value.m_path;}

         private:
            std::string m_path;
         };

         class Output : public std::ofstream
         {
         public:
            Output( std::string path);

            const std::string& path() const  { return m_path;} 
            std::string extension() const;

            inline friend std::ostream& operator << ( std::ostream& out, const Output& value) { return out << value.m_path;}

         private:
            std::string m_path;
         };

         void remove( std::string_view path);

         //! Moves/renames a file from @p source to @p target
         void move( std::string_view source, std::string_view target);

         namespace scoped
         {
            class Path
            {
            public:
               Path( std::string path);

               Path();
               ~Path();

               Path( Path&&) noexcept;
               Path& operator = ( Path&&) noexcept;

               Path( const Path&) = delete;
               Path& operator =( const Path&) = delete;


               const std::string& path() const &; // only on l-values
               operator const std::string&() const &; // only on l-values
               operator std::string_view() const &; // only on l-values

               std::string release();

               friend std::ostream& operator << ( std::ostream& out, const Path& value);

            private:
               std::string m_path;
            };


         } // scoped
         
         //! @return found paths that matches _glob_ pattern(s)
         //! explained at:
         //!   * https://man7.org/linux/man-pages/man7/glob.7.html
         //!   * https://en.wikipedia.org/wiki/Glob_(programming)
         //! @{
         std::vector< std::string> find( std::string_view pattern);
         std::vector< std::string> find( const std::vector< std::string>& patterns);
         //! @}

         //! Return the absolute path of the provided path
         //!
         //! @param path
         //! @return absolute path
         std::string absolute( std::string_view path);


         namespace name
         {
            //! @return true if path begins with /, otherwise false
            bool absolute( std::string_view path);

            //! @return a unique file-name, with post- and pre-fix, if provided
            std::string unique( std::string_view prefix = "", std::string_view postfix = "");

            //! @return filename without directory portion
            std::string base( std::string_view path);

            //! @return the extension of the file. ex. yaml for file configuration.yaml
            std::string extension( std::string_view path);

            namespace without
            {
               //! @return path without extension
               std::string extension( std::string_view path);

            } // without

            //! @return the path/name of what the link links to
            std::string link( std::string_view path);

         } // name

         //! @return true if the file exists, otherwise false
         bool exists( std::string_view path);

         namespace permission
         {
            //! @return true if calling process has execution permission on @p path
            //! @note returns false if the file does not exists
            bool execution( std::string_view path);

         } // permission
      } // file

      namespace directory
      {
         //! @return the system temporary directory. on linux /tmp
         std::string temporary();

         //! @return the current directory
         std::string current();

         //! change the WD to @p path
         //! @return previous WD
         std::string change( std::string_view path);

         namespace scope
         {
            //! Change working directory to path on construction
            //! and change back to previous WD on destruction
            struct Change
            {
               Change( std::string_view path);
               ~Change();

               Change( const Change&) = delete;
               Change& operator = ( const Change&) = delete;

            private:
               std::string m_previous;
            };

         } // scope

         namespace name
         {
            //! @return directory without filename
            std::string base( std::string_view path);
         } // name

         bool exists( std::string_view path);

         //! creates the directory path recursively
         bool create( std::string_view path);

         bool remove( std::string_view path);

      } // directory

   } // common
} // casual


