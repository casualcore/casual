//!
//! trace.h
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#ifndef TRACE_H_
#define TRACE_H_


//
// std
//
#include <string>

namespace casual
{
   namespace sf
   {
      class Trace
      {
      public:
         Trace( const std::string& information);
         ~Trace();

         Trace( const Trace&) = delete;
         Trace operator = ( const Trace&) = delete;

      private:
         const std::string m_information;
      };

   } // sf
} // casual





#endif /* TRACE_H_ */
