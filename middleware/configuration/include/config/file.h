//!
//! file.h
//!
//! Created on: Jul 1, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_CONFIG_FILE_H_
#define CASUAL_CONFIG_FILE_H_


#include <string>

namespace casual
{
   namespace config
   {

      namespace directory
      {
         //!
         //! @return configuration directory for the current domain
         //!
         std::string domain();

      } // directory

      namespace file
      {

         //!
         //! Tries to find file with supported extensions/formats
         //!
         //! @param basepath the file to be found, without extensions
         //!
         //! @return the found file.
         //!
         std::string find( const std::string& basepath);

         //!
         //! Tries to find file with supported extensions/formats
         //!
         //! @param path the path to search
         //! @param basename the file to be found, without extensions
         //!
         //! @return the found file.
         //!
         std::string find( const std::string& path, const std::string& basename);



         std::string domain();

         std::string queue();

         std::string gateway();


      } // file


   } // config


} // casual

#endif // FILE_H_
