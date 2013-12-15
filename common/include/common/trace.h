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
      class Trace
      {
      public:
         template< typename T>
         Trace( T&& info) : m_information{ std::forward< T>( info)}
         {
            log::trace << m_information << " - in" << std::endl;
         }

         ~Trace()
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

         Trace( const Trace&) = delete;
         Trace& operator = ( const Trace&) = delete;

      private:
         std::string m_information;
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
