//!
//! casual_service_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "common/service_context.h"

namespace casual
{
   namespace common
   {
      namespace service
      {

         //Context::Context() :  m_function( 0), m_called( 0) {}

         Context::Context( const std::string& name, tpservice function)
            : m_name( name), m_function( function), m_called( 0)
         {

         }

         void Context::call(TPSVCINFO* serviceInformation)
         {
            ++m_called;
            (*m_function)( serviceInformation);
         }

      } // service
   } // common
} // casual


