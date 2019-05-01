//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/file.h"

namespace casual
{
   namespace common
   {
      namespace unittest
      {

         namespace file
         {
            namespace temporary
            {
               common::file::scoped::Path content( const std::string& extension, const std::string& content);

               common::file::scoped::Path name( const std::string& extension);
            } // temporary
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

                  inline const std::string& path() const & { return m_path;}
                  inline operator const std::string&() const & { return m_path;}

                  friend std::ostream& operator << ( std::ostream& out, const Scoped& value);

               private:
                  std::string m_path;
               };
               
            } // temporary
         } // directory

      } // unittest
   } // common
} // casual


