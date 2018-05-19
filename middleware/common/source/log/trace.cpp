//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/log/trace.h"


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
               Scope::Scope( const char* information, std::ostream& log)
                  : m_information( information), m_log( log)
               {
                  if( m_log)
                  {
                     if( std::uncaught_exception())
                     {
                        log::stream::thread::Safe{ m_log} << m_information << " - in*\n";
                     }
                     else
                     {
                        log::stream::thread::Safe{ m_log} << m_information << " - in\n";
                     }
                  }
               }

               Scope::~Scope()
               {
                  if( m_log)
                  {
                     if( std::uncaught_exception())
                     {
                        log::stream::thread::Safe{ m_log} << m_information << " - out*\n";
                     }
                     else
                     {
                        log::stream::thread::Safe{ m_log} << m_information << " - out\n";
                     }
                  }
               }

               Outcome::Outcome( const char* information, std::ostream& ok, std::ostream& fail)
                  : m_information( information), m_ok( ok), m_fail( fail) {}

               Outcome::~Outcome()
               {
                  if( std::uncaught_exception())
                  {
                     if( m_fail)
                     {
                        log::stream::thread::Safe{ m_fail} << m_information << " - fail\n";
                     }
                  }
                  else
                  {
                     if( m_ok)
                     {
                        log::stream::thread::Safe{ m_ok} << m_information << " - ok\n";
                     }
                  }

               }

            } // basic

         } // trace
      } // log
   } // common
} // casual
