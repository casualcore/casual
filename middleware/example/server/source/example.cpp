//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xatmi.h"

#include "common/algorithm.h"
#include "common/domain.h"

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

            void casual_example_domain_name( TPSVCINFO* info)
            {
               auto buffer = ::tpalloc( X_OCTET, nullptr, common::domain::identity().name.size() + 1);

               common::algorithm::copy( common::domain::identity().name, buffer);
               buffer[ common::domain::identity().name.size()] = '\0';

               tpreturn( TPSUCCESS, 0, buffer, common::domain::identity().name.size() + 1, 0);
            }

            void casual_example_forward_echo( TPSVCINFO* info)
            {
               casual_service_forward( "casual/example/echo", info->data, info->len);
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

