//!
//! casual
//!

#ifndef CASUAL_UTILITY_FILE_H_
#define CASUAL_UTILITY_FILE_H_

#include <string>

#include <regex>

#include "common/move.h"

namespace casual
{

   namespace common
   {
      namespace file
      {
         void remove( const std::string& path);

         //!
         //! Moves/renames a file from @p old_path to @p new_path
         //!
         void move( const std::string& old_path, const std::string& new_path);

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


               const std::string& path() const;

               operator const std::string&() const;

               std::string release();

               friend std::ostream& operator << ( std::ostream& out, const Path& value);

            private:
               std::string m_path;
            };


         } // scoped


         //!
         //! Find the first file that matches search
         //!
         //! @param path The path to search
         //! @param search regexp to match file names
         //!
         std::string find( const std::string& path, const std::regex& search);

         //!
         //! Return the absolute path of the provided path
         //!
         //! @param path
         //! @return absolute path
         //!
         std::string absolute( const std::string& path);


         namespace name
         {
            //!
            //! @return true if path begins with /, otherwise false
            //!
            bool absolute( const std::string& path);

            //!
            //! @return a unique file-name, with post- and pre-fix, if provided
            //!
            std::string unique( const std::string& prefix = "", const std::string& postfix = "");

            //!
            //!
            //! @return filename without directory portion
            //!
            std::string base( const std::string& path);

            //!
            //! @return the extension of the file. ex. yaml for file configuration.yaml
            //!
            std::string extension( const std::string& file);

            namespace without
            {
               //!
               //!
               //! @return path without extension
               //!
               std::string extension( const std::string& path);

            } // without

         } // name


         //!
         //! @return true if the file exists, otherwise false
         //!
         bool exists( const std::string& path);

         namespace permission
         {
            //!
            //! @return true if calling process has execution permission on @p path
            //!
            //! @note returns false if the file does not exists
            //!
            bool execution( const std::string& path);

         } // permission
      } // file

      namespace directory
      {
         //!
         //! @return the system temporary directory. on linux /tmp
         //!
         std::string temporary();

         //!
         //! @return the current directory
         //!
         std::string current();

         //!
         //! change the WD to @p path
         //! @return previous WD
         //!
         std::string change( const std::string& path);

         namespace scope
         {
            //!
            //! Change working directory to path on construction
            //! and change back to previous WD on destruction
            //!
            struct Change
            {
               Change( const std::string& path);
               ~Change();

               Change( const Change&) = delete;
               Change& operator = ( const Change&) = delete;

            private:
               std::string m_previous;
            };

         } // scope

         namespace name
         {
            //!
            //!
            //! @return directory without filename
            //!
            std::string base( const std::string& path);

         } // name

         bool exists( const std::string& path);

         bool create( const std::string& path);

         bool remove( const std::string& path);

      } // directory

   } // common
} // casual

#endif /* CASUAL_UTILITY_FILE_H_ */
