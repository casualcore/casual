//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_

#include "common/trace.h"
#include "common/log.h"

namespace casual
{
   namespace common
   {
      namespace unittest
      {

         namespace detail
         {
            template< typename T>
            struct Trace
            {
               Trace( const T& test_info) : test_info( test_info)
               {
                  log::trace << "TEST( " << test_info->test_case_name() << ", " << test_info->name() << ") - in\n";
               }

               ~Trace()
               {
                  log::trace << "TEST( " << test_info->test_case_name() << ", " << test_info->name() << ") - out\n";
               }

            private:
               T test_info;
            };

         } // detail
      } // unittest
   } // common
} // casual

#define CASUAL_UNITTEST_TRACE() \
   casual::common::unittest::detail::Trace< decltype( this->test_info_)> trace{ this->test_info_}

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
