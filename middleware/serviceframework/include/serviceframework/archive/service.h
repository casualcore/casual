//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "serviceframework/service/model.h"
#include "serviceframework/archive/archive.h"



namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {
         namespace service
         {
            namespace describe
            {



               archive::Reader prepare();
               archive::Writer writer( std::vector< serviceframework::service::Model::Type>& types);


               //!
               //! To help with unittesting and such.
               //!
               struct Wrapper
               {
                  Wrapper( std::vector< serviceframework::service::Model::Type>& types) 
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
   } // serviceframework
} // casual


