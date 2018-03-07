//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_SF_ARCHIVE_SERVICE_H_
#define CASUAL_SF_ARCHIVE_SERVICE_H_

#include "sf/service/model.h"
#include "sf/archive/archive.h"



namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace service
         {
            namespace describe
            {



               archive::Reader prepare();
               archive::Writer writer( std::vector< sf::service::Model::Type>& types);


               //!
               //! To help with unittesting and such.
               //!
               struct Wrapper
               {
                  Wrapper( std::vector< sf::service::Model::Type>& types) 
                     : m_prepare( describe::prepare()), m_writer( describe::writer( types)) {}

                  template< typename T>
                  Wrapper& operator << ( T&& value)
                  {
                     m_prepare >> value;
                     m_writer << std::forward< T>( value);

                     return *this;
                  }

                  template< typename T>
                  Wrapper& operator & ( T&& value)
                  {
                     return *this << std::forward< T>( value);
                  }
               private:
                  archive::Reader m_prepare;
                  archive::Writer m_writer;
                  
               };

            } // describe

         } // service
      } // archive
   } // sf
} // casual

#endif // SERVICE_H_
