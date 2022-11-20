//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/service/type.h"
#include "common/service/invoke.h"
#include "common/executable/start.h"
#include "common/functional.h"
#include "common/string.h"

#include "casual/xa.h"
#include "casual/xatmi/defines.h"

#include <vector>
#include <string>

namespace casual
{
   namespace common
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

                  basic_service( string::Argument name, function_type function, service::transaction::Type transaction, string::Argument category)
                   : name( std::move( name)), function( std::move( function)), transaction( transaction), category( std::move( category)) {}

                  basic_service( string::Argument name, function_type function, service::transaction::Type transaction)
                     : name( std::move( name)), function( std::move( function)), transaction( transaction) {}

                  basic_service( string::Argument name, function_type function)
                     : name( std::move( name)), function( std::move( function)) {}

                  std::string name;
                  function_type function;
                  service::transaction::Type transaction = service::transaction::Type::automatic;
                  std::string category;

               };


               using Service = basic_service< std::function< service::invoke::Result( service::invoke::Parameter&&)>>;


               namespace xatmi
               {
                  using Service = basic_service< std::function< void( TPSVCINFO*)>>;

               } // xatmi

               namespace transaction
               {
                  using Resource = executable::argument::transaction::Resource;
               } // transaction

            } // argument

            void start( std::vector< argument::Service> services, std::vector< argument::transaction::Resource> resources);
            void start( std::vector< argument::Service> services);


            //! Start an XATMI server. Will call the callback @p initialize before "ready" is sent
            //! to casual, if provided.
            void start(
               std::vector< argument::xatmi::Service> services,
               std::vector< argument::transaction::Resource> resources,
               common::function<void()const> initialize);

         } // v1
      } // server
   } // common
} // casual


