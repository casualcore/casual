//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/server/context.h"
#include "common/server/argument.h"


#include "common/message/server.h"
#include "common/service/call/context.h"


#include "common/communication/instance.h"
#include "common/buffer/pool.h"
#include "common/process.h"

#include "common/log.h"
#include "common/log/category.h"
#include "common/log.h"


#include <algorithm>

namespace casual
{
   namespace common
   {
      namespace server
      {

         namespace state
         {
            std::ostream& operator << ( std::ostream& out, const Jump& value)
            {
               return out << "{ value: " << value.state.value
                     << ", code: " << value.state.code
                     << ", data: @" << static_cast< void*>( value.buffer.data)
                     << ", size: " << value.buffer.size
                     << ", service: " << value.forward.service << '}';
            }
         } // state


         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }



         Context::Context()
         {
            Trace log{ "server::Context instantiated"};
         }


         void Context::jump_return( flag::xatmi::Return rval, long rcode, char* data, long len)
         {
            // Prepare buffer.
            // We have to keep state, since there seems not to be any way to send information
            // via longjump...

            m_state.jump.state.value = rval;
            m_state.jump.state.code = rcode;
            m_state.jump.buffer.data = data;
            m_state.jump.buffer.size = len;
            m_state.jump.forward.service.clear();

            log::line( log::debug, "Context::jump_return - jump state: ", m_state.jump);

            std::longjmp( m_state.jump.environment, state::Jump::Location::c_return);
         }


         void Context::forward( const char* service, char* data, long size)
         {
            m_state.jump.state.value = flag::xatmi::Return::success;
            m_state.jump.state.code = 0;
            m_state.jump.buffer.data = data;
            m_state.jump.buffer.size = size;

            m_state.jump.forward.service = service ? service : "";

            log::line( log::debug, "Context::forward - jump state: ", m_state.jump);

            std::longjmp( m_state.jump.environment, state::Jump::Location::c_forward);
         }

         void Context::advertise( const std::string& service, void (*adress)( TPSVCINFO *))
         {
            Trace trace{ "server::Context::advertise"};

            auto prospect = xatmi::service( service, adress);

            // validate
            if( prospect.name.size() >= XATMI_SERVICE_NAME_LENGTH)
            {
               prospect.name.resize( XATMI_SERVICE_NAME_LENGTH - 1);
               log::line( log::category::warning, "service name '", service, "' truncated to '", prospect.name, "'");
            }

            auto found = algorithm::find( m_state.services, prospect.name);

            if( found)
            {
               // service name is already advertised
               // No error if it's the same function
               if( found->second != prospect)
               {
                  throw common::exception::xatmi::service::Advertised( "service name: " + prospect.name);
               }
            }
            else
            {
               message::service::Advertise message;
               message.process = process::handle();

               auto found = algorithm::find_if( m_state.physical_services, [&]( const Service& s){
                  return s == prospect;
               });

               if( found)
               {
                  m_state.services.emplace( prospect.name, *found);
                  message.services.emplace_back( prospect.name, found->category, found->transaction);
               }
               else
               {
                  message.services.emplace_back( prospect.name, prospect.category, prospect.transaction);

                  m_state.physical_services.push_back( prospect);
                  m_state.services.emplace( prospect.name, m_state.physical_services.back());
               }
               communication::ipc::blocking::send( communication::instance::outbound::service::manager::device(), message);
            }
         }

         void Context::unadvertise( const std::string& service)
         {
            Trace log{ "server::Context::unadvertise"};

            if( m_state.services.erase( service) != 1)
            {
               throw common::exception::xatmi::service::no::Entry( "service name: " + service);
            }

            message::service::Advertise message;
            message.directive = message::service::Advertise::Directive::remove;
            message.process = process::handle();
            message.services.emplace_back( service);

            communication::ipc::blocking::send( communication::instance::outbound::service::manager::device(), message);
         }


         void Context::configure( const server::Arguments& arguments)
         {
            Trace log{ "server::Context::configure"};

            for( auto& service : arguments.services)
            {
               m_state.physical_services.push_back( service);
               m_state.services.emplace(
                     service.name,
                     m_state.physical_services.back());
            }
         }

         namespace local
         {
            namespace
            {
               template< typename S, typename P>
               server::Service* find_physical( S& services, P&& predicate)
               {
                  auto found = algorithm::find_if( services, predicate);

                  if( found)
                  {
                     return &( *found);
                  }
                  return nullptr;
               }
            } // <unnamed>
         } // local

         server::Service* Context::physical( const std::string& name)
         {
            return local::find_physical(  m_state.physical_services, [&]( const server::Service& s){
               return s.name == name;
            });
         }

         server::Service* Context::physical( const server::xatmi::function_type& function)
         {
            return local::find_physical(  m_state.physical_services, [&]( const server::Service& s){
               return s == xatmi::address( function);
            });

         }

         State& Context::state()
         {
            return m_state;
         }


         void Context::finalize()
         {
            buffer::pool::Holder::instance().clear();
            execution::service::clear();
            execution::service::parent::clear();
         }



      } // server

   } // common
} // casual

