//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "common/server/context.h"


#include "common/message/server.h"
#include "common/call/context.h"
#include "common/call/timeout.h"


#include "common/queue.h"
#include "common/buffer/pool.h"
#include "common/process.h"

#include "common/log.h"
#include "common/error.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"




#include <algorithm>



namespace casual
{
   namespace common
   {
      namespace server
      {
         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }



         Context::Context()
         {
            trace::internal::Scope log{ "server::Context instansiated"};
         }


         void Context::long_jump_return( int rval, long rcode, char* data, long len, long flags)
         {
            log::internal::debug << "tpreturn - rval: " << rval << " - rcode: " << rcode << " - data: @" << static_cast< void*>( data) << " - len: " << len << " - flags: " << flags << std::endl;

            //
            // Prepare buffer.
            // We have to keep state, since there seems not to be any way to send information
            // via longjump...
            //

            m_state.jump.state.value = rval;
            m_state.jump.state.code = rcode;
            m_state.jump.state.data = data;
            m_state.jump.state.len = len;

            longjmp( m_state.long_jump_buffer, 1);
         }

         void Context::advertise( const std::string& service, void (*adress)( TPSVCINFO *))
         {
            trace::internal::Scope trace{ "server::Context advertise service " + service};

            Service prospect{ service, adress};


            //
            // validate
            //


            if( prospect.name.size() >= XATMI_SERVICE_NAME_LENGTH)
            {
               prospect.name.resize( XATMI_SERVICE_NAME_LENGTH - 1);
               log::warning << "service name '" << service << "' truncated to '" << prospect.name << "'";
            }

            auto found = range::find( m_state.services, prospect.name);

            if( found)
            {
               //
               // service name is already advertised
               // No error if it's the same function
               //
               if( found->second != prospect)
               {
                  throw common::exception::xatmi::service::AllreadyAdvertised( "service name: " + prospect.name);
               }

            }
            else
            {
               // TODO: find the function and get information from it (transaction semantics and such)

               message::service::Advertise message;

               message.process = process::handle();
               message.serverPath = process::path();
               message.services.emplace_back( prospect.name);

               // TODO: make it consistence safe...
               queue::blocking::Writer writer( ipc::broker::id());
               writer( message);

               m_state.services.emplace( prospect.name, std::move( prospect));
            }
         }

         void Context::unadvertise( const std::string& service)
         {
            trace::internal::Scope log{ "server::Context unadvertise service" + service};

            if( m_state.services.erase( service) != 1)
            {
               throw common::exception::xatmi::service::NoEntry( "service name: " + service);
            }

            message::service::Unadvertise message;
            message.process = process::handle();
            message.services.push_back( message::Service( service));

            queue::blocking::Writer writer( ipc::broker::id());
            writer( message);
         }


         State& Context::state()
         {
            return m_state;
         }


         void Context::finalize()
         {
            call::Timeout::instance().clear();
            buffer::pool::Holder::instance().clear();
            call::Context::instance().service( "");
         }



      } // server

   } // common
} // casual

