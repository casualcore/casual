//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/category.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace log
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
               Scope( const char (&information)[size], std::ostream& log)
                  : basic::Scope( information, log) {}
            };


            class Outcome : public basic::Outcome
            {
            public:
               template<decltype(sizeof("")) size>
               Outcome( const char (&information)[size], std::ostream& ok = log::category::information, std::ostream& fail = log::category::error)
                  : basic::Outcome( information, ok, fail) {}
            };


         } // trace

         using Trace = trace::Scope;

      } // log
   } // common
} // casual




