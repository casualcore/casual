//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi/server.h"
#include "casual/xatmi/internal/log.h"
#include "casual/xatmi/internal/code.h"
#include "casual/xatmi/internal/transform.h"


#include "common/server/start.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"

#include "common/functional.h"
#include "common/process.h"
#include "common/event/send.h"
#include "common/signal.h"


#include <vector>
#include <algorithm>


namespace casual
{
   namespace xatmi
   {
      namespace server
      {
         namespace local
         {
            namespace
            {
               namespace transform
               {

                  template< typename A>
                  auto services( A& value)
                  {
                     std::vector< common::server::argument::xatmi::Service> result;

                     auto service = value.services;

                     for( ; service->function_pointer != nullptr; ++service)
                     {
                        result.emplace_back(
                           service->name,
                           service->function_pointer,
                           common::service::transaction::mode( service->transaction),
                           service->category);
                     }

                     return result;
                  }

               } // transform

               template< typename A> 
               int start( const A& argument)
               {
                  return common::exception::main::guard( [&argument]()
                  {
                     casual::xatmi::Trace trace{ "casual::xatmi::server::local::start"};

                     auto done = common::execute::scope( [done = argument.server_done]()
                     {
                        if( done)
                           common::invoke( done);
                     });

                     // We block child so users can spawn stuff without actions/errors from casual
                     common::signal::thread::scope::Block block( { common::code::signal::child});

                     common::server::start(
                        transform::services( argument),
                        xatmi::transform::resources( argument.xa_switches),
                        [&]()
                        {
                           if( argument.server_init && common::invoke( argument.server_init, argument.argc, argument.argv) == -1)
                           {      
                              // if init is not ok, then we don't call done, symmetry with ctor/dtor
                              done.release();
                              common::event::error::raise( common::code::xatmi::argument, "server initialize failed - action: exit");
                           }
                        });
                  });
               }

            } // <unnamed>
         } // local
      } // server
   } // xatmi
} // casual


int casual_start_server( struct casual_server_argument* arguments)
{
   return casual::xatmi::server::local::start( *arguments);
}

int casual_run_server( struct casual_server_arguments* arguments)
{
    return casual::xatmi::server::local::start( *arguments);
}








