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
      class base_trace
      {
      protected:
         template< typename T>
         base_trace( std::ostream& log, T&& info) : m_log( log), m_information{ std::forward< T>( info)}
         {
            if( m_log.good())
            {
               log::trace << m_information << " - in" << std::endl;
            }
         }

         ~base_trace()
         {
            if( m_log.good())
            {
               if( std::uncaught_exception())
               {
                  log::trace << m_information << " - out*" << std::endl;
               }
               else
               {
                  log::trace << m_information << " - out" << std::endl;
               }
            }
         }

         base_trace( const base_trace&) = delete;
         base_trace& operator = ( const base_trace&) = delete;

      private:
         std::ostream& m_log;
         std::string m_information;
      };

      class Trace : base_trace
      {
      public:
         template< typename T>
         Trace( T&& info) : base_trace( log::trace, std::forward< T>( info)) {}
      };

      namespace trace
      {
         struct Exit
         {
         public:
            template< typename T>
            Exit( T&& information) : m_information{ std::forward< T>( information)} {}

            ~Exit()
           {
              if( std::uncaught_exception())
              {
                 log::trace << m_information << " - failed" << std::endl;
              }
              else
              {
                 log::trace << m_information << " - ok" << std::endl;
              }
           }

         private:
            std::string m_information;
         };
      }
   }


}



#endif /* TRACE_H_ */
