//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "server/service/invoke.h"

#include "transaction/resource/link.h"

#include "common/service/type.h"

#include "common/functional.h"
#include "common/string.h"

#include "casual/xa.h"
#include "casual/xatmi/defines.h"

#include <vector>
#include <string>

namespace casual
{
   namespace server
   {
      inline namespace v1
      {
         namespace argument
         {
            template< typename F>
            struct basic_service
            {
               using function_type = F;

               std::string name;
               function_type function;
               common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
               common::service::visibility::Type visibility = common::service::visibility::Type::discoverable;
               std::string category;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( transaction);
                  CASUAL_SERIALIZE( visibility);
                  CASUAL_SERIALIZE( category);
               );

            };


            using Service = basic_service< std::function< service::invoke::Result( service::invoke::Parameter&&)>>;


            namespace xatmi
            {
               //using Service = basic_service< std::function< void( TPSVCINFO*)>>;

            } // xatmi

            namespace transaction
            {
               using Resource = casual::transaction::resource::Link;
            } // transaction

         } // argument

         void start( std::vector< argument::Service> services, std::vector< argument::transaction::Resource> resources);
         void start( std::vector< argument::Service> services);

      } // v1

   } // server
} // casual


