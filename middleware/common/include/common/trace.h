//!
//! casual
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
         namespace basic
         {
            class Scope
            {
            public:
               ~Scope();
            protected:
               Scope( const char* information, std::ostream& log);
            private:
               const char* const m_information;
               std::ostream& m_log;
            };

            class Outcome
            {
            public:
               ~Outcome();
            protected:
               Outcome( const char* information, std::ostream& ok, std::ostream& fail);
            private:
               const char* const m_information;
               std::ostream& m_ok;
               std::ostream& m_fail;
            };
         } // basic


         class Scope : public basic::Scope
         {
         public:
            template<decltype(sizeof("")) size>
            Scope( const char (&information)[size], std::ostream& log = log::trace)
               : basic::Scope( information, log) {}
         };


         class Outcome : public basic::Outcome
         {
         public:
            template<decltype(sizeof("")) size>
            Outcome( const char (&information)[size], std::ostream& ok = log::information, std::ostream& fail = log::error)
               : basic::Outcome( information, ok, fail) {}
         };


      } // trace

      using Trace = trace::Scope;

   } // common
} // casual



#endif /* TRACE_H_ */
