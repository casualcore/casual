//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_START_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_START_H_


#include "common/service/type.h"
#include "common/service/invoke.h"

#include "xa.h"
#include "xatmi/defines.h"

#include <functional>
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

                  basic_service( std::string name, function_type function, service::transaction::Type transaction, std::string category)
                   : name( std::move( name)), function( std::move( function)), transaction( transaction), category( std::move( category)) {}

                  basic_service( std::string name, function_type function, service::transaction::Type transaction)
                     : name( std::move( name)), function( std::move( function)), transaction( transaction) {}

                  basic_service( std::string name, function_type function)
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
                  struct Resource
                  {
                     Resource( std::string key, xa_switch_t* xa_switch)
                      : key( std::move( key)), xa_switch( xa_switch) {}

                     std::string key;
                     xa_switch_t* xa_switch = nullptr;
                  };

               } // transaction

            } // argument

            void start( std::vector< argument::Service> services, std::vector< argument::transaction::Resource> resources);
            void start( std::vector< argument::Service> services);

            //!
            //! Start an XATMI server. Will call the callback @p connected when connection has established
            //! with casual, if provided.
            //!
            //! @param services
            //! @param resources
            //! @param connected
            void start(
                  std::vector< argument::xatmi::Service> services,
                  std::vector< argument::transaction::Resource> resources,
                  std::function<void()> connected);


            int main( int argc, char **argv, std::function< void( int, char**)> local_main);

         } // v1

      } // server
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_START_H_
