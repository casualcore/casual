//!
//! trace.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "sf/trace.h"

#include "common/logger.h"

namespace casual
{
   namespace sf
   {

      Trace::Trace( const std::string& information) : m_information( information)
      {
         common::logger::trace << " " << m_information << " IN";
      }

      Trace::~Trace()
      {
         common::logger::trace << " " << m_information << " OUT";
      }


   } // sf
} // casual






