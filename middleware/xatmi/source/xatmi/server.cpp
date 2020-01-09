//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi/server.h"
#include "casual/xatmi/internal/log.h"
#include "casual/xatmi/internal/transform.h"


#include "common/server/start.h"


#include "common/functional.h"
#include "common/exception/xatmi.h"
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
                  try
                  {
                     casual::xatmi::Trace trace{ "casual::xatmi::server::local::start"};

                     // We block child so users can spawn stuff without actions/errors from casual
                     common::signal::thread::scope::Block block( { common::code::signal::child});

                     common::server::start(
                           transform::services( argument),
                           xatmi::transform::resources( argument.xa_switches),
                           [&](){
                              if( argument.server_init)
                              {
                                 if( common::invoke( argument.server_init, argument.argc, argument.argv) == -1)
                                 {
                                    common::event::error::send( "server initialize failed - action: exit");
                                    throw common::exception::xatmi::invalid::Argument{ "server initialize failed"};
                                 }
                              }
                     });

                     if( argument.server_done)
                     {
                        common::invoke( argument.server_done);
                     }

                  }
                  catch( ...)
                  {
                     return static_cast< int>( casual::common::exception::xatmi::handle());
                  }
                  return 0;
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








