//!
//! casual_utility_file.h
//!
//! Created on: May 5, 2012
//!     Author: Lazan
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

         namespace scoped
         {
            class Path
            {
            public:
               Path( const std::string& path);
               //Path();
               ~Path();

               Path( Path&&);

               Path( const Path&) = delete;
               Path& operator =( const Path&) = delete;


               const std::string& path() const;

               operator const std::string&() const;

               void release() { m_moved.release();}

            private:

               std::string m_path;
               move::Moved m_moved;
            };


         } // scoped


         //!
         //! @return a unique file-name, with post- and pre-fix, if provided
         //!
         std::string unique( const std::string& prefix = "", const std::string& postfix = "");

         //!
         //! Find the first file that matches search
         //!
         //! @param path The path to search
         //! @param search regexp to match file names
         //!
         std::string find( const std::string& path, const std::regex& search);

         //!
         //!
         //! @return filename without directory portion
         //!
         std::string basename( const std::string& path);

         //!
         //!
         //! @return directory without filename
         //!
         std::string basedir( const std::string& path);

         //!
         //!
         //! @return path without extension
         //!
         std::string removeExtension( const std::string& path);

         //!
         //! @return the extension of the file. ex. yaml for file configuration.yaml
         //!
         std::string extension( const std::string& file);

         //!
         //! @return true if the file exists, otherwise false
         //!
         bool exists( const std::string& path);

      } // file

      namespace directory
      {

         bool create( const std::string& path);

         bool remove( const std::string& path);

      }

   } // common
} // casual

#endif /* CASUAL_UTILITY_FILE_H_ */
