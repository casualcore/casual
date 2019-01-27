//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include <string>

namespace casual
{
   namespace configuration
   {

      namespace directory
      {
         //! @return configuration directory for the current domain
         std::string domain();

         std::string persistent();

      } // directory

      namespace file
      {

         //! Tries to find file with supported extensions/formats
         //!
         //! @param basepath the file to be found, without extensions
         //!
         //! @return the found file.
         std::string find( const std::string& basepath);

         //! Tries to find file with supported extensions/formats
         //!
         //! @param path the path to search
         //! @param basename the file to be found, without extensions
         //!
         //! @return the found file.
         std::string find( const std::string& path, const std::string& basename);


         namespace persistent
         {
            std::string domain();
         } // persistent

      } // file


   } // config


} // casual


