//!
//! trace.h
//!
//! Created on: Nov 25, 2012
//!     Author: Lazan
//!

#ifndef TRACE_H_
#define TRACE_H_

#include "common/log.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace trace
      {
         namespace internal
         {
            class basic
            {
            public:
               basic( std::string information, std::ostream& log = log::trace);

            protected:
               std::string m_information;
               std::ostream& m_log;

            };


         } // internal

         class Scope : public internal::basic
         {
         public:
            Scope( std::string information, std::ostream& log = log::trace);
            ~Scope();
         };


         class Outcome : public internal::basic
         {
         public:
            Outcome( std::string information, std::ostream& ok = log::information, std::ostream& fail = log::error);
            ~Outcome();

         private:
            std::ostream& m_fail;
         };


      } // trace

      using Trace = trace::Scope;

   } // common
} // casual



#endif /* TRACE_H_ */
