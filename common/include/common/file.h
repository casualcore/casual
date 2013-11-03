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

namespace casual
{

   namespace common
   {
      namespace file
      {
         void remove( const std::string& path);

         class RemoveGuard
         {
         public:
            RemoveGuard( const std::string& path);
            ~RemoveGuard();

            RemoveGuard( RemoveGuard&&);

            RemoveGuard( const RemoveGuard&) = delete;
            RemoveGuard& operator =( const RemoveGuard&) = delete;

            const std::string& path() const;

            void release() { m_released = true; };

         private:

            std::string m_path;
            bool m_released = false;
         };

         class ScopedPath: public RemoveGuard
         {
         public:
            using RemoveGuard::RemoveGuard;

            operator const std::string&();
         };

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

      }

   } // common
} // casual

#endif /* CASUAL_UTILITY_FILE_H_ */
