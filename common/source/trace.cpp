//!
//! trace.cpp
//!
//! Created on: Sep 17, 2014
//!     Author: Lazan
//!

#include "common/trace.h"


namespace casual
{
   namespace common
   {
      namespace trace
      {
         namespace internal
         {

            basic::basic( std::string information, std::ostream& log)
              : m_information( std::move( information)), m_log( log) {}

         } // internal


         Scope::Scope( std::string information, std::ostream& log)
         : internal::basic( std::move( information), log)
         {
            if( m_log)
            {
               if( std::uncaught_exception())
               {
                  log::thread::Safe{ m_log} << m_information << " - in*\n";
               }
               else
               {
                  log::thread::Safe{ m_log} << m_information << " - in\n";
               }
            }
         }

         Scope::~Scope()
         {
            if( m_log)
            {
               if( std::uncaught_exception())
               {
                  log::thread::Safe{ m_log} << m_information << " - out*\n";
               }
               else
               {
                  log::thread::Safe{ m_log} << m_information << " - out\n";
               }
            }
         }

         Outcome::Outcome( std::string information, std::ostream& ok, std::ostream& fail)
          : internal::basic( std::move( information), ok), m_fail( ok) {}

         Outcome::~Outcome()
         {
            if( std::uncaught_exception())
            {
               if( m_fail)
               {
                  log::thread::Safe{ m_fail} << m_information << " - fail\n";
               }
            }
            else
            {
               if( m_log)
               {
                  log::thread::Safe{ m_log} << m_information << " - ok\n";
               }
            }
         }
      } // trace
   } // common
} // casual
