//!
//! casual 
//!

#include "common/server/handle/service.h"

#include "common/server/context.h"
#include "common/buffer/pool.h"


namespace casual
{

   namespace common
   {
      namespace server
      {
         namespace handle
         {
            namespace service
            {
               namespace transform
               {
                  message::service::call::Reply reply( const message::service::call::callee::Request& message)
                  {
                     message::service::call::Reply result;

                     result.correlation = message.correlation;
                     result.descriptor = message.descriptor;
                     result.buffer = buffer::Payload{ nullptr};
                     result.error = TPESVCERR;

                     return result;
                  }

                  message::conversation::send::caller::Request reply( const message::conversation::connect::callee::Request& message)
                  {
                     message::conversation::send::caller::Request result{ buffer::Payload{ nullptr}};

                     result.correlation = message.correlation;
                     result.buffer = buffer::Payload{ nullptr};
                     //result.error = TPESVCERR;

                     return result;
                  }


                  TPSVCINFO information( message::service::call::callee::Request& message)
                  {
                     Trace trace{ "server::handle::transform::information"};

                     TPSVCINFO result;

                     //
                     // Before we call the user function we have to add the buffer to the "buffer-pool"
                     //
                     //range::copy_max( message.service.name, )
                     strncpy( result.name, message.service.name.c_str(), sizeof( result.name) );
                     result.len = message.buffer.memory.size();
                     result.cd = message.descriptor;
                     result.flags = message.flags;

                     //
                     // This is the only place where we use adopt
                     //
                     result.data = buffer::pool::Holder::instance().adopt( std::move( message.buffer));

                     return result;
                  }

                  TPSVCINFO information( message::conversation::connect::callee::Request& message)
                  {
                     return {};
                  }

               } // transform

               namespace complement
               {
                  void reply( message::service::call::Reply& reply, const server::state::Jump& jump)
                  {
                     reply.code = jump.state.code;
                     reply.error = jump.state.value == TPSUCCESS ? 0 : TPESVCFAIL;

                     if( jump.buffer.data != nullptr)
                     {
                        try
                        {
                           reply.buffer = buffer::pool::Holder::instance().release( jump.buffer.data, jump.buffer.size);
                        }
                        catch( ...)
                        {
                           error::handler();
                           reply.error = TPESVCERR;
                        }
                     }
                     else
                     {
                        reply.buffer = buffer::Payload{ nullptr};
                     }
                  }

                  void reply( message::conversation::send::caller::Request& reply, const server::state::Jump& jump)
                  {
                     auto event = []( int value){
                        return value == TPSUCCESS ? common::service::conversation::Event::service_success :
                              common::service::conversation::Event::service_fail;
                     };

                     reply.events = event( jump.state.value);

                     if( jump.buffer.data != nullptr)
                     {
                        try
                        {
                           reply.buffer = buffer::pool::Holder::instance().release( jump.buffer.data, jump.buffer.size);
                        }
                        catch( ...)
                        {
                           error::handler();
                           reply.events = common::service::conversation::Event::service_error;
                        }
                     }
                     else
                     {
                        reply.buffer = buffer::Payload{ nullptr};
                     }

                  }

               } // complement
            }
         }

      } // server
   } // common

} // casual
