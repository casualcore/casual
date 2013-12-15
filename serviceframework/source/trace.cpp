//!
//! trace.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "sf/trace.h"

#include "common/log.h"

namespace casual
{
   namespace sf
   {

      Trace::Trace( const std::string& information) : m_information( information)
      {
         common::log::trace << " " << m_information << " IN" << std::endl;
      }

      Trace::~Trace()
      {
         common::log::trace << " " << m_information << " OUT" << std::endl;
      }


   } // sf
} // casual






