//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "common/server/context.h"


#include "common/message/server.h"
#include "common/call/context.h"


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

         std::ostream& operator << ( std::ostream& out, const State::jump_t& value)
         {
            return out << "{ value: " << value.state.value << ", code: " << value.state.code << ", data: @" << static_cast< void*>( value.buffer.data)
               << ", len: " << value.buffer.len << ", service: " << value.forward.service << '}';
         }

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
            //
            // Prepare buffer.
            // We have to keep state, since there seems not to be any way to send information
            // via longjump...
            //

            m_state.jump.state.value = rval;
            m_state.jump.state.code = rcode;
            m_state.jump.buffer.data = data;
            m_state.jump.buffer.len = len;
            m_state.jump.forward.service.clear();

            log::internal::debug << "Context::long_jump_return - jump state: " << m_state.jump << '\n';

            longjmp( m_state.long_jump_buffer, State::jump_t::From::c_return);
         }


         void Context::forward( const char* service, char* data, long size)
         {
            m_state.jump.state.value = 0;
            m_state.jump.state.code = 0;
            m_state.jump.buffer.data = data;
            m_state.jump.buffer.len = size;

            m_state.jump.forward.service = service ? service : "";

            log::internal::debug << "Context::forward - jump state: " << m_state.jump << '\n';

            longjmp( m_state.long_jump_buffer, State::jump_t::From::c_forward);
         }

         void Context::advertise( const std::string& service, void (*adress)( TPSVCINFO *))
         {
            //trace::internal::Scope trace{ "server::Context advertise service: " + service};
            trace::internal::Scope trace{ "server::Context::advertise"};

            Service prospect{ service, adress};

            //
            // validate
            //
            if( prospect.origin.size() >= XATMI_SERVICE_NAME_LENGTH)
            {
               prospect.origin.resize( XATMI_SERVICE_NAME_LENGTH - 1);
               log::warning << "service name '" << service << "' truncated to '" << prospect.origin << "'";
            }



            auto found = range::find( m_state.services, prospect.origin);

            if( found)
            {
               //
               // service name is already advertised
               // No error if it's the same function
               //
               if( found->second != prospect)
               {
                  throw common::exception::xatmi::service::Advertised( "service name: " + prospect.origin);
               }
            }
            else
            {
               message::service::Advertise message;

               message.process = process::handle();
               message.serverPath = process::path();

               auto found = range::find_if( m_state.physical_services, [&]( const Service& s){
                  return s == prospect;
               });

               if( found)
               {
                  m_state.services.emplace( prospect.origin, *found);
                  message.services.emplace_back( prospect.origin, found->type, service::transaction::mode( found->transaction));
               }
               else
               {
                  message.services.emplace_back( prospect.origin, prospect.type, service::transaction::mode( prospect.transaction));

                  m_state.physical_services.push_back( std::move( prospect));
                  m_state.services.emplace( prospect.origin, m_state.physical_services.back());
               }

               queue::blocking::Writer writer( ipc::broker::id());
               writer( message);
            }
         }

         void Context::unadvertise( const std::string& service)
         {
            //trace::internal::Scope log{ "server::Context unadvertise service: " + service};
            trace::internal::Scope log{ "server::Context::unadvertise"};

            if( m_state.services.erase( service) != 1)
            {
               throw common::exception::xatmi::service::no::Entry( "service name: " + service);
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
            buffer::pool::Holder::instance().clear();
            execution::service( "");
            execution::parent::service( "");
         }



      } // server

   } // common
} // casual

