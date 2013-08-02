//!
//! trace.h
//!
//! Created on: Nov 25, 2012
//!     Author: Lazan
//!

#ifndef TRACE_H_
#define TRACE_H_

#include "common/logger.h"

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
            logger::trace << m_information << " - in";
         }

         ~Trace()
         {
            if( std::uncaught_exception())
            {
               logger::trace << m_information << " - out*";
            }
            else
            {
               logger::trace << m_information << " - out";
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
                 logger::trace << m_information << " - failed";
              }
              else
              {
                 logger::trace << m_information << " - ok";
              }
           }

         private:
            std::string m_information;
         };
      }
   }


}



#endif /* TRACE_H_ */
