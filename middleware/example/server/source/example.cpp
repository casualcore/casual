//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xatmi.h"

#include "common/algorithm.h"

#include <locale>

namespace casual
{

   namespace example
   {
      namespace server
      {

         extern "C"
         {

            void casual_example_echo( TPSVCINFO* info)
            {
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

            void casual_example_conversation( TPSVCINFO* info)
            {
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

            void casual_example_sink( TPSVCINFO* info)
            {
               tpreturn( TPSUCCESS, 0, nullptr, 0, 0);
            }
            
            void casual_example_uppercase( TPSVCINFO* info)
            {
               auto buffer = common::range::make( info->data, info->len);

               common::algorithm::transform( buffer, buffer, ::toupper);

               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

            void casual_example_lowercase( TPSVCINFO* info)
            {
               auto buffer = common::range::make( info->data, info->len);

               common::algorithm::transform( buffer, buffer, ::tolower);

               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

            void casual_example_rollback( TPSVCINFO* info)
            {
               tpreturn( TPFAIL, 0, info->data, info->len, 0);
            }

            void casual_example_error_system( TPSVCINFO* info)
            {
               throw std::runtime_error{ "throwing from service"};
            }

            void casual_example_terminate( TPSVCINFO* info)
            {
               std::terminate();
            }
         }
      } // server
   } // example
} // casual

